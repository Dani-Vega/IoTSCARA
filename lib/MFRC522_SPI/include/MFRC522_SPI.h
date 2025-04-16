#ifndef MFRC522_SPI_H
#define MFRC522_SPI_H

#include <SPIManager.h>

class MFRC522_SPI
{
private:
    enum MIFARE_Misc
    {
        MF_ACK = 0xA,
        MF_KEY_SIZE = 6
    };

    typedef struct
    {
        uint8_t size;
        uint8_t uidByte[10];
        uint8_t sak;
    } Uid;

    // Registros del MFRC522. Descritos en el capítulo 9 de la datasheet.
    enum PCD_Register
    {
        // Page 0: Command and status
        //						  0x00			// A usar en el futuro
        CommandReg = 0x01 << 1,    // Empieza y detiene la ejecucuón de un comando
        ComIEnReg = 0x02 << 1,     // Activa/Desactiva la interrupción de control de bits de solicitud
        DivIEnReg = 0x03 << 1,     // Activa/Desactiva la interrupción de control de bits de solicitud
        ComIrqReg = 0x04 << 1,     // Interrupción de bits de solicitud
        DivIrqReg = 0x05 << 1,     // Interrupción de bits de solicitud
        ErrorReg = 0x06 << 1,      // Bits para mostrar el status de error del útlimo comando ejecutado
        Status1Reg = 0x07 << 1,    // Bits de status de comunicaión
        Status2Reg = 0x08 << 1,    // Receiver/Transmitter bits de status
        FIFODataReg = 0x09 << 1,   // Input/Output de buffer FIFO 64 bytes
        FIFOLevelReg = 0x0A << 1,  // Número de bytes guardados en el buffer FIFO
        WaterLevelReg = 0x0B << 1, // Level for FIFO underflow and overflow warning
        ControlReg = 0x0C << 1,    // miscellaneous control registers
        BitFramingReg = 0x0D << 1, // adjustments for bit-oriented frames
        CollReg = 0x0E << 1,       // bit position of the first bit-collision detected on the RF interface
                                   //						  0x0F			// A usar en el futuro

        // Page 1: Command
        // 						  0x10			// A usar en el futuro
        ModeReg = 0x11 << 1,        // defines general modes for transmitting and receiving
        TxModeReg = 0x12 << 1,      // defines transmission data rate and framing
        RxModeReg = 0x13 << 1,      // defines reception data rate and framing
        TxControlReg = 0x14 << 1,   // controls the logical behavior of the antenna driver pins TX1 and TX2
        TxASKReg = 0x15 << 1,       // controls the setting of the transmission modulation
        TxSelReg = 0x16 << 1,       // selects the internal sources for the antenna driver
        RxSelReg = 0x17 << 1,       // selects internal receiver settings
        RxThresholdReg = 0x18 << 1, // selects thresholds for the bit decoder
        DemodReg = 0x19 << 1,       // defines demodulator settings
                                    // 						  0x1A			// reserved for future use
                                    // 						  0x1B			// reserved for future use
        MfTxReg = 0x1C << 1,        // controls some MIFARE communication transmit parameters
        MfRxReg = 0x1D << 1,        // controls some MIFARE communication receive parameters
                                    // 						  0x1E			// reserved for future use
        SerialSpeedReg = 0x1F << 1, // selects the speed of the serial UART interface

        // Page 2: Configuration
        // 						  0x20			// reserved for future use
        CRCResultRegH = 0x21 << 1, // shows the MSB and LSB values of the CRC calculation
        CRCResultRegL = 0x22 << 1,
        // 						  0x23			// reserved for future use
        ModWidthReg = 0x24 << 1,   // controls the ModWidth setting?
                                   // 						  0x25			// reserved for future use
        RFCfgReg = 0x26 << 1,      // configures the receiver gain
        GsNReg = 0x27 << 1,        // selects the conductance of the antenna driver pins TX1 and TX2 for modulation
        CWGsPReg = 0x28 << 1,      // defines the conductance of the p-driver output during periods of no modulation
        ModGsPReg = 0x29 << 1,     // defines the conductance of the p-driver output during periods of modulation
        TModeReg = 0x2A << 1,      // defines settings for the internal timer
        TPrescalerReg = 0x2B << 1, // the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
        TReloadRegH = 0x2C << 1,   // defines the 16-bit timer reload value
        TReloadRegL = 0x2D << 1,
        TCounterValueRegH = 0x2E << 1, // shows the 16-bit timer value
        TCounterValueRegL = 0x2F << 1,

        // Page 3: Test Registers
        // 						  0x30			// reserved for future use
        TestSel1Reg = 0x31 << 1,     // general test signal configuration
        TestSel2Reg = 0x32 << 1,     // general test signal configuration
        TestPinEnReg = 0x33 << 1,    // enables pin output driver on pins D1 to D7
        TestPinValueReg = 0x34 << 1, // defines the values for D1 to D7 when it is used as an I/O bus
        TestBusReg = 0x35 << 1,      // shows the status of the internal test bus
        AutoTestReg = 0x36 << 1,     // controls the digital self-test
        VersionReg = 0x37 << 1,      // shows the software version
        AnalogTestReg = 0x38 << 1,   // controls the pins AUX1 and AUX2
        TestDAC1Reg = 0x39 << 1,     // defines the test value for TestDAC1
        TestDAC2Reg = 0x3A << 1,     // defines the test value for TestDAC2
        TestADCReg = 0x3B << 1       // shows the value of ADC I and Q channels
                                     // 						  0x3C			// reserved for production tests
                                     // 						  0x3D			// reserved for production tests
                                     // 						  0x3E			// reserved for production tests
                                     // 						  0x3F			// reserved for production tests
    };

    // MFRC522 commands. Described in chapter 10 of the datasheet.
    enum PCD_Command
    {
        PCD_Idle = 0x00,             // no action, cancels current command execution
        PCD_Mem = 0x01,              // stores 25 bytes into the internal buffer
        PCD_GenerateRandomID = 0x02, // generates a 10-byte random ID number
        PCD_CalcCRC = 0x03,          // activates the CRC coprocessor or performs a self-test
        PCD_Transmit = 0x04,         // transmits data from the FIFO buffer
        PCD_NoCmdChange = 0x07,      // no command change, can be used to modify the CommandReg register bits without affecting the command, for example, the PowerDown bit
        PCD_Receive = 0x08,          // activates the receiver circuits
        PCD_Transceive = 0x0C,       // transmits data from FIFO buffer to antenna and automatically activates the receiver after transmission
        PCD_MFAuthent = 0x0E,        // performs the MIFARE standard authentication as a reader
        PCD_SoftReset = 0x0F         // resets the MFRC522
    };

    // MFRC522 RxGain[2:0] masks, defines the receiver's signal voltage gain factor (on the PCD).
    enum PCD_RxGain
    {
        RxGain_18dB = 0x00 << 4,   // 000b - 18 dB, minimum
        RxGain_23dB = 0x01 << 4,   // 001b - 23 dB
        RxGain_18dB_2 = 0x02 << 4, // 010b - 18 dB, it seems 010b is a duplicate for 000b
        RxGain_23dB_2 = 0x03 << 4, // 011b - 23 dB, it seems 011b is a duplicate for 001b
        RxGain_33dB = 0x04 << 4,   // 100b - 33 dB, average, and typical default
        RxGain_38dB = 0x05 << 4,   // 101b - 38 dB
        RxGain_43dB = 0x06 << 4,   // 110b - 43 dB
        RxGain_48dB = 0x07 << 4,   // 111b - 48 dB, maximum
        RxGain_min = 0x00 << 4,    // 000b - 18 dB, minimum, convenience for RxGain_18dB
        RxGain_avg = 0x04 << 4,    // 100b - 33 dB, average, convenience for RxGain_33dB
        RxGain_max = 0x07 << 4     // 111b - 48 dB, maximum, convenience for RxGain_48dB
    };

    // Commands sent to the PICC.
    enum PICC_Command
    {
        // The commands used by the PCD to manage communication with several PICCs (ISO 14443-3, Type A, section 6.4)
        PICC_CMD_REQA = 0x26,          // REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
        PICC_CMD_WUPA = 0x52,          // Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
        PICC_CMD_CT = 0x88,            // Cascade Tag. Not really a command, but used during anti collision.
        PICC_CMD_SEL_CL1 = 0x93,       // Anti collision/Select, Cascade Level 1
        PICC_CMD_SEL_CL2 = 0x95,       // Anti collision/Select, Cascade Level 2
        PICC_CMD_SEL_CL3 = 0x97,       // Anti collision/Select, Cascade Level 3
        PICC_CMD_HLTA = 0x50,          // HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
        PICC_CMD_RATS = 0xE0,          // Request command for Answer To Reset.
                                       // The commands used for MIFARE Classic (from http://www.mouser.com/ds/2/302/MF1S503x-89574.pdf, Section 9)
                                       // Use PCD_MFAuthent to authenticate access to a sector, then use these commands to read/write/modify the blocks on the sector.
                                       // The read/write commands can also be used for MIFARE Ultralight.
        PICC_CMD_MF_AUTH_KEY_A = 0x60, // Perform authentication with Key A
        PICC_CMD_MF_AUTH_KEY_B = 0x61, // Perform authentication with Key B
        PICC_CMD_MF_READ = 0x30,       // Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
        PICC_CMD_MF_WRITE = 0xA0,      // Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
        PICC_CMD_MF_DECREMENT = 0xC0,  // Decrements the contents of a block and stores the result in the internal data register.
        PICC_CMD_MF_INCREMENT = 0xC1,  // Increments the contents of a block and stores the result in the internal data register.
        PICC_CMD_MF_RESTORE = 0xC2,    // Reads the contents of a block into the internal data register.
        PICC_CMD_MF_TRANSFER = 0xB0,   // Writes the contents of the internal data register to a block.
                                       // The commands used for MIFARE Ultralight (from http://www.nxp.com/documents/data_sheet/MF0ICU1.pdf, Section 8.6)
                                       // The PICC_CMD_MF_READ and PICC_CMD_MF_WRITE can also be used for MIFARE Ultralight.
        PICC_CMD_UL_WRITE = 0xA2       // Writes one 4 byte page to the PICC.
    };

    // last value set to 0xff, then compiler uses less ram, it seems some optimisations are triggered
    typedef enum
    {
        PICC_TYPE_UNKNOWN,
        PICC_TYPE_ISO_14443_4,        // PICC compliant with ISO/IEC 14443-4
        PICC_TYPE_ISO_18092,          // PICC compliant with ISO/IEC 18092 (NFC)
        PICC_TYPE_MIFARE_MINI,        // MIFARE Classic protocol, 320 bytes
        PICC_TYPE_MIFARE_1K,          // MIFARE Classic protocol, 1KB
        PICC_TYPE_MIFARE_4K,          // MIFARE Classic protocol, 4KB
        PICC_TYPE_MIFARE_UL,          // MIFARE Ultralight or Ultralight C
        PICC_TYPE_MIFARE_PLUS,        // MIFARE Plus
        PICC_TYPE_MIFARE_DESFIRE,     // MIFARE DESFire
        PICC_TYPE_TNP3XXX,            // Only mentioned in NXP AN 10833 MIFARE Type Identification Procedure
        PICC_TYPE_NOT_COMPLETE = 0xff // SAK indicates UID is not complete.
    } PICC_Type;

    enum StatusCode
    {
        STATUS_OK,                // Success
        STATUS_ERROR,             // Error in communication
        STATUS_COLLISION,         // Collission detected
        STATUS_TIMEOUT,           // Timeout in communication.
        STATUS_NO_ROOM,           // A buffer is not big enough.
        STATUS_INTERNAL_ERROR,    // Internal error in the code. Should not happen ;-)
        STATUS_INVALID,           // Invalid argument.
        STATUS_CRC_WRONG,         // The CRC_A does not match
        STATUS_MIFARE_NACK = 0xff // A MIFARE PICC responded with NAK.
    };

    spi_device_handle_t spi;
    gpio_num_t pinRST;
    gpio_num_t pinCs;

    Uid uid;
    enum StatusCode state;

    // Métodos para escribir registros
    void PCD_WriteRegister(uint8_t Register, uint8_t value);
    void PCD_WriteRegisterMany(uint8_t Register, uint8_t count, uint8_t *values);
    // Métodos para leer registros
    uint8_t PCD_ReadRegister(uint8_t Register);
    void PCD_ReadRegisterMany(uint8_t Register, uint8_t count, uint8_t *values, uint8_t rxAlign);
    // Métodos para setear o borrar bits en específico
    void PCD_ClearRegisterBitMask(uint8_t reg, uint8_t mask);
    void PCD_SetRegisterBitMask(uint8_t reg, uint8_t mask);

    void PCD_AntennaOn();

    uint8_t PICC_RequestA(uint8_t *bufferATQA, uint8_t *bufferSize);
    uint8_t PICC_REQA_or_WUPA(uint8_t command, uint8_t *bufferATQA, uint8_t *bufferSize);
    uint8_t PCD_TransceiveData(uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC);
    uint8_t PCD_CommunicateWithPICC(uint8_t command, uint8_t waitIRq, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen, uint8_t *validBits, uint8_t rxAlign, bool checkCRC);

    uint8_t PCD_CalculateCRC(uint8_t *data, uint8_t length, uint8_t *result);
    uint8_t PICC_HaltA();

    void PICC_GetTypeName(PICC_Type piccType);
    PICC_Type PICC_GetType(uint8_t sak);

    void GetStatusCodeName(uint8_t code); ///< One of the StatusCode enums.

    uint8_t PCD_MIFARE_Transceive(uint8_t *sendData, ///< Pointer to the data to transfer to the FIFO. Do NOT include the CRC_A.
                                  uint8_t sendLen,   ///< Number of bytes in sendData.
                                  bool acceptTimeout ///< True => A timeout is also success
    );

public:
    // Estructura usada para pasar una clave MIFARE Crypto1
    typedef struct
    {
        uint8_t keyByte[MF_KEY_SIZE];
    } MIFARE_Key;

    MFRC522_SPI();
    ~MFRC522_SPI();
    void PCD_setup(SPIManager &spiManager, const std::string &deviceName, gpio_num_t cs, gpio_num_t rst);
    void PCD_Version();
    bool PICC_IsNewCardPresent();
    uint8_t PICC_ReadCardSerial();

    uint8_t PICC_Select(uint8_t validBits);
    void PCD_StopCrypto1();

    uint8_t MIFARE_Read(uint8_t blockAddr,  ///< MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The first page to return data from.
                     uint8_t *buffer,    ///< The buffer to store the data in
                     uint8_t *bufferSize ///< Buffer size, at least 18 bytes. Also number of bytes returned if STATUS_OK.
    );

    uint8_t MIFARE_Write(uint8_t blockAddr, ///< MIFARE Classic: The block (0-0xff) number. MIFARE Ultralight: The page (2-15) to write to.
                         uint8_t *buffer,   ///< The 16 bytes to write to the PICC
                         uint8_t bufferSize ///< Buffer size, must be at least 16 bytes. Exactly 16 bytes are written.
    );

    void PICC_DumpMifareUltralightToSerial();

    std::string getUIDString();

    void PICC_DumpToSerial();

    uint8_t PCD_Authenticate(uint8_t command,   ///< PICC_CMD_MF_AUTH_KEY_A or PICC_CMD_MF_AUTH_KEY_B
                             uint8_t blockAddr, ///< The block number. See numbering in the comments in the .h file.
                             MIFARE_Key *key    ///< Pointer to the Crypteo1 key to use (6 bytes)
    );
    void PICC_DumpMifareClassicToSerial(PICC_Type piccType, ///< One of the PICC_Type enums.
                                        MIFARE_Key *key     ///< Key A used for all sectors.
    );
    void PICC_DumpDetailsToSerial();
    void PICC_DumpMifareClassicSectorToSerial(MIFARE_Key *key, ///< Key A for the sector.
                                              uint8_t sector   ///< The sector to dump, 0..39.
    );
};

#endif // __MFRC522_SPI_H__