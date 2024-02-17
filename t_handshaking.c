#include <stdio.h>
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

bool tryHandshake(HANDLE hSerial) {
    // Clear any previous data
    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    // Wait for the handshake message from Arduino
    DWORD bytesRead;
    char buffer[32];
    // Read may return immediately if there's no data, due to the setup of COMMTIMEOUTS
    BOOL readSuccess = ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    if (!readSuccess || bytesRead == 0) {
        return false; // Handshake failed, no data read
    }
    buffer[bytesRead] = '\0'; // Null-terminate the string

    // Check for handshake message
    if (strstr(buffer, "HANDSHAKE") != NULL) {
        // If handshake received, send ACK back to Arduino
        const char* ackMsg = "ACK\n";
        DWORD bytesWritten;
        WriteFile(hSerial, ackMsg, strlen(ackMsg), &bytesWritten, NULL);
        return true;
    }
    return false; // Handshake failed, incorrect data
}

HANDLE autoDetectArduinoPort() {
    HANDLE hSerial = INVALID_HANDLE_VALUE;
    for (int i = 1; i <= 256; i++) {
        char portName[20];
        sprintf(portName, "\\\\.\\COM%d", i);
        printf("Testing %s\n", portName);

        hSerial = CreateFileA(portName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
        if (hSerial == INVALID_HANDLE_VALUE) {
            continue; // Unable to open port, try next
        }

        // Set up the device control block (DCB) for the serial port
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(DCB);
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            continue; // Unable to get the comm state, try next
        }

        // Configure the serial port settings
        dcbSerialParams.BaudRate = CBR_9600;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            continue; // Unable to set the comm state, try next
        }

        // Set the timeouts for the serial port
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant = 5000; // Set to 5 seconds
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 5000; // Also 5 seconds for writes
        timeouts.WriteTotalTimeoutMultiplier = 0;
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            CloseHandle(hSerial);
            continue; // Unable to set timeouts, try next
        }

        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
        Sleep(2000); // Give Arduino time to reset

        if (tryHandshake(hSerial)) {
            printf("Arduino detected on %s\n", portName);
            return hSerial;
        } else {
            printf("Handshake failed on %s\n", portName);
            CloseHandle(hSerial); // Handshake failed, close and try next
        }
    }
    return INVALID_HANDLE_VALUE; // Arduino not found
}

int main() {
    printf("Starting Arduino detection...\n");
    HANDLE hSerial = autoDetectArduinoPort();
    while(hSerial == INVALID_HANDLE_VALUE){
        printf("Arduino not found\n");
        printf("Trying again\n");
        hSerial = autoDetectArduinoPort();
    }
    CloseHandle(hSerial);
    return 0;
}

