#include "DFPlayerMini.h"

DFPlayerMini::DFPlayerMini() {}

DFPlayerMini::~DFPlayerMini() {}

void DFPlayerMini::setTimeOut(unsigned long timeOutDuration)
{
	_timeOutDuration = timeOutDuration;
}

void DFPlayerMini::uint16ToArray(uint16_t value, uint8_t *array)
{
	*array = (uint8_t)(value >> 8);
	*(array + 1) = (uint8_t)(value);
}

uint16_t DFPlayerMini::calculateCheckSum(uint8_t *buffer)
{
	uint16_t sum = 0;
	for (int i = Stack_Version; i < Stack_CheckSum; i++)
	{
		sum += buffer[i];
	}
	return -sum;
}

void DFPlayerMini::sendStack()
{
	ESP_LOGD(__FUNCTION__, "_sending[Stack_ACK]=%d _isSending=%d", _sending[Stack_ACK], _isSending);
	if (_sending[Stack_ACK])
	{ // if the ack mode is on wait until the last transmition
		while (_isSending)
		{
			delay(0);
			ESP_LOGD(__FUNCTION__, "available()");
			available();
		}
	}

	if (_isDebug)
	{
		printf("\n");
		printf("sending:");
		for (int i = 0; i < DFPLAYER_SEND_LENGTH; i++)
		{
			printf("%x", _sending[i]);
			printf(" ");
		}
		printf("\n");
	}
	serial.write_buffer(_sending, DFPLAYER_SEND_LENGTH);
	_timeOutTimer = millis();
	_isSending = _sending[Stack_ACK];

	if (!_sending[Stack_ACK])
	{ // if the ack mode is off wait 10 ms after one transmition.
		delay(10);
	}
	ESP_LOGD(__FUNCTION__, "end");
}

void DFPlayerMini::sendStack1(uint8_t command)
{ // sendStack->sendStack1
	sendStack2(command, 0);
}

void DFPlayerMini::sendStack2(uint8_t command, uint16_t argument)
{ // sendStack->sendStack2
	_sending[Stack_Command] = command;
	uint16ToArray(argument, _sending + Stack_Parameter);
	uint16ToArray(calculateCheckSum(_sending), _sending + Stack_CheckSum);
	sendStack();
}

void DFPlayerMini::sendStack3(uint8_t command, uint8_t argumentHigh, uint8_t argumentLow)
{ // sendStack->sendStack3
	uint16_t buffer = argumentHigh;
	buffer <<= 8;
	sendStack2(command, buffer | argumentLow);
}

void DFPlayerMini::enableACK()
{
	_sending[Stack_ACK] = 0x01;
}

void DFPlayerMini::disableACK()
{
	_sending[Stack_ACK] = 0x00;
}

bool DFPlayerMini::waitAvailable(unsigned long duration)
{
	ESP_LOGD(__FUNCTION__, "start");
	unsigned long timer = millis();
	if (!duration)
	{
		duration = _timeOutDuration;
	}
	while (!available())
	{
		ESP_LOGD(__FUNCTION__, "!available()");
		if (millis() - timer > duration)
		{
			return false;
		}
		// delay(0);
		delay(1);
	}
	ESP_LOGD(__FUNCTION__, "end");
	return true;
}

bool DFPlayerMini::setup(int txd, int rxd, bool isACK, bool doReset, bool debug)
{
	_isDebug = debug;
	if (_isInstall == false)
	{
		serial.setup(9600, txd, rxd);
		_isInstall = true;
	}

	if (isACK)
	{
		enableACK();
	}
	else
	{
		disableACK();
	}

	if (doReset)
	{
		reset();
		waitAvailable(2000);
		delay(200);
	}
	else
	{
		// assume same state as with reset(): online
		_handleType = DFPlayerCardOnline;
	}

	return (readType() == DFPlayerCardOnline) || (readType() == DFPlayerUSBOnline) || !isACK;
}

uint8_t DFPlayerMini::readType()
{
	_isAvailable = false;
	return _handleType;
}

uint16_t DFPlayerMini::read()
{
	_isAvailable = false;
	return _handleParameter;
}

bool DFPlayerMini::handleMessage(uint8_t type, uint16_t parameter)
{
	_receivedIndex = 0;
	_handleType = type;
	_handleParameter = parameter;
	_isAvailable = true;
	return _isAvailable;
}

bool DFPlayerMini::handleError(uint8_t type, uint16_t parameter)
{
	handleMessage(type, parameter);
	_isSending = false;
	return false;
}

uint8_t DFPlayerMini::readCommand()
{
	_isAvailable = false;
	return _handleCommand;
}

void DFPlayerMini::parseStack()
{
	uint8_t handleCommand = *(_received + Stack_Command);
	if (handleCommand == 0x41)
	{ // handle the 0x41 ack feedback as a spcecial case, in case the pollusion of _handleCommand, _handleParameter, and _handleType.
		_isSending = false;
		return;
	}

	_handleCommand = handleCommand;
	_handleParameter = arrayToUint16(_received + Stack_Parameter);
	ESP_LOGD(__FUNCTION__, "_handleCommand=0x%x _handleParameter=0x%x", _handleCommand, _handleParameter);

	switch (_handleCommand)
	{
	case 0x3D:
		handleMessage(DFPlayerPlayFinished, _handleParameter);
		break;
	case 0x3F:
		if (_handleParameter & 0x01)
		{
			handleMessage(DFPlayerUSBOnline, _handleParameter);
		}
		else if (_handleParameter & 0x02)
		{
			handleMessage(DFPlayerCardOnline, _handleParameter);
		}
		else if (_handleParameter & 0x03)
		{
			handleMessage(DFPlayerCardUSBOnline, _handleParameter);
		}
		break;
	case 0x3A:
		if (_handleParameter & 0x01)
		{
			handleMessage(DFPlayerUSBInserted, _handleParameter);
		}
		else if (_handleParameter & 0x02)
		{
			handleMessage(DFPlayerCardInserted, _handleParameter);
		}
		break;
	case 0x3B:
		if (_handleParameter & 0x01)
		{
			handleMessage(DFPlayerUSBRemoved, _handleParameter);
		}
		else if (_handleParameter & 0x02)
		{
			handleMessage(DFPlayerCardRemoved, _handleParameter);
		}
		break;
	case 0x40:
		handleMessage(DFPlayerError, _handleParameter);
		break;
	case 0x3C:
	case 0x3E:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F:
		handleMessage(DFPlayerFeedBack, _handleParameter);
		break;
	default:
		handleError(WrongStack, 0);
		break;
	}
}

uint16_t DFPlayerMini::arrayToUint16(uint8_t *array)
{
	uint16_t value = *array;
	value <<= 8;
	value += *(array + 1);
	return value;
}

bool DFPlayerMini::validateStack()
{
	return calculateCheckSum(_received) == arrayToUint16(_received + Stack_CheckSum);
}

bool DFPlayerMini::available()
{
	while (serial.available())
	{
		delay(0);
		if (_receivedIndex == 0)
		{
			_received[Stack_Header] = serial.read();
			if (_isDebug)
			{
				printf("received:");
				printf("%x", _received[_receivedIndex]);
				printf(" ");
			}
			if (_received[Stack_Header] == 0x7E)
			{
				_receivedIndex++;
			}
		}
		else
		{
			_received[_receivedIndex] = serial.read();
			if (_isDebug)
			{
				printf("%x", _received[_receivedIndex]);
				printf(" ");
			}
			switch (_receivedIndex)
			{
			case Stack_Version:
				if (_received[_receivedIndex] != 0xFF)
				{
					return handleError(WrongStack, 0);
				}
				break;
			case Stack_Length:
				if (_received[_receivedIndex] != 0x06)
				{
					return handleError(WrongStack, 0);
				}
				break;
			case Stack_End:
				if (_isDebug)
				{
					printf("\n");
				}
				if (_received[_receivedIndex] != 0xEF)
				{
					return handleError(WrongStack, 0);
				}
				else
				{
					if (validateStack())
					{
						_receivedIndex = 0;
						parseStack();
						return _isAvailable;
					}
					else
					{
						return handleError(WrongStack, 0);
					}
				}
				break;
			default:
				break;
			}
			_receivedIndex++;
		}
	}

	if (_isSending && (millis() - _timeOutTimer >= _timeOutDuration))
	{
		return handleError(TimeOut, 0);
	}

	return _isAvailable;
}

void DFPlayerMini::next()
{
	sendStack1(0x01);
}

void DFPlayerMini::previous()
{
	sendStack1(0x02);
}

void DFPlayerMini::play(int fileNumber)
{
	sendStack2(0x03, fileNumber);
}

void DFPlayerMini::volumeUp()
{
	sendStack1(0x04);
}

void DFPlayerMini::volumeDown()
{
	sendStack1(0x05);
}

void DFPlayerMini::volume(uint8_t volume)
{
	sendStack2(0x06, volume);
}

void DFPlayerMini::EQ(uint8_t eq)
{
	sendStack2(0x07, eq);
}

void DFPlayerMini::loop(int fileNumber)
{
	sendStack2(0x08, fileNumber);
}

void DFPlayerMini::outputDevice(uint8_t device)
{
	sendStack2(0x09, device);
	delay(200);
}

void DFPlayerMini::sleep()
{
	sendStack1(0x0A);
}

void DFPlayerMini::reset()
{
	sendStack1(0x0C);
}

void DFPlayerMini::resume()
{
	sendStack1(0x0D);
}

void DFPlayerMini::pause()
{
	sendStack1(0x0E);
}

void DFPlayerMini::playFolder(uint8_t folderNumber, uint8_t fileNumber)
{
	sendStack3(0x0F, folderNumber, fileNumber);
}

void DFPlayerMini::outputSetting(bool enable, uint8_t gain)
{
	sendStack3(0x10, enable, gain);
}

void DFPlayerMini::enableLoopAll()
{
	sendStack2(0x11, 0x01);
}

void DFPlayerMini::disableLoopAll()
{
	sendStack2(0x11, 0x00);
}

void DFPlayerMini::playMp3Folder(int fileNumber)
{
	sendStack2(0x12, fileNumber);
}

void DFPlayerMini::advertise(int fileNumber)
{
	sendStack2(0x13, fileNumber);
}

void DFPlayerMini::playLargeFolder(uint8_t folderNumber, uint16_t fileNumber)
{
	sendStack2(0x14, (((uint16_t)folderNumber) << 12) | fileNumber);
}

void DFPlayerMini::stopAdvertise()
{
	sendStack1(0x15);
}

void DFPlayerMini::stop()
{
	sendStack1(0x16);
}

void DFPlayerMini::loopFolder(int folderNumber)
{
	sendStack2(0x17, folderNumber);
}

void DFPlayerMini::randomAll()
{
	sendStack1(0x18);
}

void DFPlayerMini::enableLoop()
{
	sendStack2(0x19, 0x00);
}

void DFPlayerMini::disableLoop()
{
	sendStack2(0x19, 0x01);
}

void DFPlayerMini::enableDAC()
{
	sendStack2(0x1A, 0x00);
}

void DFPlayerMini::disableDAC()
{
	sendStack2(0x1A, 0x01);
}

uint16_t DFPlayerMini::readState()
{
	sendStack1(0x42);
	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readVolume()
{
	sendStack1(0x43);
	if (waitAvailable(0))
	{
		return read();
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readEQ()
{
	sendStack1(0x44);
	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readFileCounts(uint8_t device)
{
	switch (device)
	{
	case DFPLAYER_DEVICE_U_DISK:
		sendStack1(0x47);
		break;
	case DFPLAYER_DEVICE_SD:
		sendStack1(0x48);
		break;
	case DFPLAYER_DEVICE_FLASH:
		sendStack1(0x49);
		break;
	default:
		break;
	}

	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readCurrentFileNumber(uint8_t device)
{
	switch (device)
	{
	case DFPLAYER_DEVICE_U_DISK:
		sendStack1(0x4B);
		break;
	case DFPLAYER_DEVICE_SD:
		sendStack1(0x4C);
		break;
	case DFPLAYER_DEVICE_FLASH:
		sendStack1(0x4D);
		break;
	default:
		break;
	}
	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readFileCountsInFolder(int folderNumber)
{
	sendStack2(0x4E, folderNumber);
	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int DFPlayerMini::readFolderCounts()
{
	sendStack1(0x4F);
	if (waitAvailable(0))
	{
		if (readType() == DFPlayerFeedBack)
		{
			return read();
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

bool DFPlayerMini::isFinished(int *value)
{
	static int finishd = 0;
	if (available())
	{
		uint8_t type = readType();
		*value = read();
		if (_isDebug)
		{
			printf("type=%d value=%d finishd=%d\n", type, *value, finishd);
		}
		if (type == DFPlayerPlayFinished)
		{
			finishd++;
		}
		if (finishd == 2)
		{
			finishd = 0;
			return true;
		}
	}
	return false;
}

void DFPlayerMini::printDetail(uint8_t type, int value)
{
	switch (type)
	{
	case TimeOut:
		printf("Time Out!\n");
		break;
	case WrongStack:
		printf("Stack Wrong!\n");
		break;
	case DFPlayerCardInserted:
		printf("Card Inserted!\n");
		break;
	case DFPlayerCardRemoved:
		printf("Card Removed!\n");
		break;
	case DFPlayerCardOnline:
		printf("Card Online!\n");
		break;
	case DFPlayerPlayFinished:
		printf("Number:%d Play Finished!\n", value);
		break;
	case DFPlayerError:
		printf("DFPlayerError:");
		switch (value)
		{
		case Busy:
			printf("Card not found\n");
			break;
		case Sleeping:
			printf("Sleeping\n");
			break;
		case SerialWrongStack:
			printf("Get Wrong Stack\n");
			break;
		case CheckSumNotMatch:
			printf("Check Sum Not Match\n");
			break;
		case FileIndexOut:
			printf("File Index Out of Bound\n");
			break;
		case FileMismatch:
			printf("Cannot Find File\n");
			break;
		case Advertise:
			printf("In Advertise\n");
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
