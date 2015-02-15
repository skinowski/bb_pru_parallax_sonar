/*
 * sonar.cpp
 *
 *  Created on: Dec 20, 2014
 *      Author: tceylan
 */

#include "sonar.h"

#include <unistd.h>
#include <poll.h>
#include <errno.h>

 // Driver header file
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "sonar_common.hp"

namespace robo {

// Which PRU are we running on?
#define PRU_NUM 0
#define BINARY_NAME "./sonar.bin"

static uint64_t soundspeed_usec2cm(uint64_t usec, float temp)
{
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the
    // object we take half of the distance travelled.
    // http://www.parallax.com/sites/default/files/downloads/28015-PING-Documentation-v1.6.pdf
    const float speed = 331.5f + (0.6f * temp); // m/sec
    return usec / (10000.0f / speed);
}

#define HANDLE_EINTR(x) ({                   \
    typeof(x) _result;                       \
    do                                       \
        _result = (x);                       \
    while (_result == -1 && errno == EINTR); \
    _result;                                 \
})


Sonar::Sonar()
	:
    m_addr(0),
    m_fd(-1),
    m_temperature(20.0f), // room temp in C
    m_trx(0),
    m_pending(false),
    m_is_pru_init(false),
    m_is_pru_enabled(false),
    m_is_pru_int_enabled(false)
{
}

Sonar::~Sonar()
{
    shutdown();
}

void Sonar::set_temperature(float temp)
{
    m_temperature = temp;
}

int Sonar::initialize()
{
    if (m_is_pru_init)
        return EFAULT;

    int ret = 0;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    void *pruDataMem = 0;

    /* Initialize the PRU */
    ret = prussdrv_init();
    if (ret)
    	goto error;
    m_is_pru_init = true;

    /* Open PRU Interrupt */
    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
    	goto error;
    m_is_pru_enabled = true;

    /* Get the interrupt initialized */
    ret = prussdrv_pruintc_init(&pruss_intc_initdata);
    if (ret)
    	goto error;
    m_is_pru_int_enabled = true;

    // clear out any events
    if (PRU_NUM == 0)
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    else
        prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);

	// fetch the fd for polling
    if (PRU_NUM == 0)
    	m_fd = prussdrv_pru_event_fd(PRU_EVTOUT_0);
    else
    	m_fd = prussdrv_pru_event_fd(PRU_EVTOUT_1);

    if (m_fd == -1)
    {
    	if (errno)
            ret = errno;
	    else
            ret = -1;
    }
    if (ret)
    	goto error;

    // Initialize pointer to PRU data memory
    if (PRU_NUM == 0)
      ret = prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &pruDataMem);
    else
      ret = prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, &pruDataMem);

    if (ret)
    	goto error;

    m_addr = static_cast<unsigned int*>(pruDataMem);
    m_pending = false;

    // Flush the values in the PRU data memory locations
    m_addr[ADDR_REQUEST_ID_IDX]         = get_trx();
    m_addr[ADDR_RESPONSE_IDX]           = 0x00;
    m_addr[ADDR_RESPONSE_STATUS_IDX]    = 0x00;

    /* Execute example on PRU */
    ret = prussdrv_exec_program(PRU_NUM, BINARY_NAME);
    if (ret)
    	goto error;

    return 0;

    error:
        shutdown();
    return ret;
}

uint32_t Sonar::get_trx()
{
    while (++m_trx == REQUEST_EXIT);
	return m_trx;	
}

void Sonar::shutdown()
{
    if (m_is_pru_int_enabled)
    {
        clear_event();
        m_is_pru_int_enabled = false;
    }

    if (m_is_pru_enabled)
    {
        /* Disable PRU and close memory mapping*/
        prussdrv_pru_disable(PRU_NUM);
        m_is_pru_enabled = false;
    }

    if (m_is_pru_init)
    {
        prussdrv_exit();
        m_is_pru_init = false;
    }

    if (m_fd != -1)
    {
        HANDLE_EINTR(::close(m_fd));
        m_fd = -1;
    }

    m_pending = false;
    m_addr = 0;
}

int Sonar::clear_event()
{
    int ret = 0;
    if (PRU_NUM == 0)
        ret = prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    else
        ret = prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
    return ret;
}

int Sonar::trigger()
{
    if (m_pending)
        return EBUSY;

    int ret = clear_event();
    if (ret)
        return ret;

    // simple, just update the pru memory with new trx
    const uint32_t trx = get_trx();
    m_addr[ADDR_REQUEST_ID_IDX] = trx;
    m_pending = true;
    return ret;
}

uint64_t Sonar::get_cm_distance(uint64_t nsecs) const
{
    const uint64_t usec = nsecs / 1000;
    return soundspeed_usec2cm(usec, m_temperature);
}

int Sonar::fetch_result(uint64_t &distance_cm)
{
    if (!m_pending)
        return EINVAL;

    int err = 0;

    // TODO: depending on overall robot codebase, try to factor out file
    // descriptor polling to master loop where these are polled in a set
    // not individually like below...
    struct pollfd fd;
    fd.fd = m_fd;
    fd.events = POLLPRI | POLLIN;

    int ret = HANDLE_EINTR(::poll(&fd, 1, 0));
    err = errno;

    if (ret == 0)
        return EAGAIN;

    // we check for *anything* in revents.. 
    // This will allow us to pull any relevant error codes from ::read() for the fd.
    if (ret != 1 || !fd.revents)
        return err;

    // we are no longer pending even if the ::read() fails below... 
    // no need to stay and retry reads...
    m_pending = false;

    unsigned int event_count = 0;
    ret = HANDLE_EINTR(::read(m_fd, &event_count, sizeof(int)));
    err = errno;
    if (ret != sizeof(event_count))
        return EFAULT;

    const uint32_t status = m_addr[ADDR_RESPONSE_STATUS_IDX];
    if (status == RESULT_OK)
    {
        // nsecs
        const uint64_t reading = m_addr[ADDR_RESPONSE_IDX] * RESULT_UNITS_NSECS;
        distance_cm = get_cm_distance(reading);
        clear_event();
        return 0;
    }
    
    return err ? err : EFAULT;
}

} // namespace robo


