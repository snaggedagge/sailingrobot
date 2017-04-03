
#ifndef canpgnreciever_h
#define canpgnreciever_h

#include <stdint.h>
#include <vector>

class CanPGNReceiver
{

public:
	virtual void processPGN(std::vector<uint8_t> Data, uint32_t PGN) = 0;
};

#endif
