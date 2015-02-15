
#include "sonar.h"
#include "errno.h"
#include <stdio.h>

int main(void)
{
	int ret = 0;

	robo::Sonar sonar;

	ret = sonar.initialize();
	if (ret)
		return ret;

	sonar.set_temperature(20.0f);

	// issue a request
	ret = sonar.trigger();
	if (ret)
		return ret;

	// Simple busy loop example
	//
	// TODO: Add timeout here. (eg. 100-200 msec)
	// Actual parallax ping))) response is max 18.5 msecs, but total time
	// will vary due to possible linux scheduling delays.
	do
	{
		uint64_t result = 0;
		ret = sonar.fetch_result(result);
		if (!ret)
			printf("Sonar distance=%llu cm\n", result);
	}
	while (ret == EAGAIN);

	sonar.shutdown();
	return ret;
}