/*
 * sonar.h
 *
 *  Created on: Dec 20, 2014
 *      Author: tceylan
 */

#ifndef SONAR_H_
#define SONAR_H_

#include <stdint.h>

namespace robo {

class Sonar
{
public:
    Sonar();
    ~Sonar();

    int initialize();
    void shutdown();

    int trigger();
    int fetch_result(uint64_t &distance_cm);

    void set_temperature(float temp);

private:
    // no copy
    Sonar(const Sonar &);
    Sonar& operator=(const Sonar &);

    uint32_t get_trx();
    uint64_t get_cm_distance(uint64_t nsecs) const;

    int clear_event();

private:
    unsigned int 	*m_addr;
    int 			m_fd;
    float 			m_temperature;
    uint32_t 		m_trx;
    bool 			m_pending;
    bool 			m_is_pru_init;
    bool 			m_is_pru_enabled;
    bool 			m_is_pru_int_enabled;
};

} // namespace robo


#endif /* SONAR_H_ */
