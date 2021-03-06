/********************************************************
 * Description                                          
 *      File name : UARTfunctions.c
 *      Project   : Lab_5_Ex2
 *      Author(s) : O.N�meth
 *      Device    : Firecracker
 *      Function  :               
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018
 *      Rev       : 1.0
 *      Alt A     :
 ********************************************************/

/***** HEADER FILES *****/
#include"UARTfunctions.h"
#include"gpIO.h"
#include<string.h>                  // For using strlen
#include"PWMfunctions.h"
#include<stdlib.h>                  // For using strtol and memset

#define lenstr 100                  // Define string length

/***** FUNCTIONS *****/

/********************************************************
 * Description                                          
 *      Name      : UART2Setup                                                 
 *      Author(s) : O.Nemeth                                                  
 *      Function  : Set up function for UART2 for 
 *                  communication between USB devices 
 *                  and the PIC              
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018                          
 *      Rev       : 1.0                                 
 *      Alt A     :                                     
 ********************************************************/
void UART2Setup(void){
U2MODEbits.UARTEN   = 0;      // Disable UART during config
U2BRG               = 16;     // Choose appropriate baud rate
U2MODEbits.LPBACK   = 0;      // Disable loopback mode
U2MODEbits.WAKE     = 0;      // Do not wake-up on serial activity (dont care) 
U2MODEbits.ABAUD    = 0;      // No auto-baud detection
U2MODEbits.PDSEL    = 0;      // 8 databits, no parity
U2MODEbits.STSEL    = 0;      // One stop bit
U2STAbits.URXISEL   = 0;      // Receive interrupt when 1 character arrives
// For RX (Receive)
IFS1bits.U2RXIF     = 0;      // Clear rx interrupt flag
IPC6bits.U2RXIP     = 3;      // Set receive interrupt priority
IEC1bits.U2RXIE     = 1;      // Enable receive interupts
// For TX (Transmit))
IFS1bits.U2TXIF     = 0;      // Clear tx interrupt flag
IPC6bits.U2TXIP     = 3;
IEC1bits.U2TXIE     = 1;      // Enable UART Transmit interrupt
U2STAbits.UTXEN     = 1;      // Enable UART Tx

U2MODEbits.UARTEN   = 1;      // Now, enable UART!
U2STAbits.UTXEN     = 1;      // And enable transmission (order important)
}   
 /********************************************************
 * Description                                          
 *      Name      : mySendString                                                
 *      Author(s) : O.Nemeth                                                  
 *      Function  : Writes a string to the PC's terminal
 *                  via UART and the U2TXREG.              
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018                          
 *      Rev       : 1.0                                 
 *      Alt A     :                                    
 ********************************************************/

void mySendString(char *textString){           
static int i, len;
len = strlen(textString);                           // Determine the string length
    for (i = 0 ; i < len; i++) { 
        while (U2STAbits.UTXBF);                    // Wait while Tx buffer full
        U2TXREG = textString[i];                    // Print string 
    }
}  
 /********************************************************
 * Description                                          
 *      Name      : decode                                                
 *      Author(s) : O.Nemeth                                                  
 *      Function  : Decodes the input char, prints to PC
 *                  terminal "OK: pulseWidth " if the char
 *                  is between 0 - 100 when using start bit
 *                  '<' and stop bit '>'. Any other strings
 *                  cause an "ERROR! " message to be printed                
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018                          
 *      Rev       : 1.0                                 
 *      Alt A     :            
 ********************************************************/
void decode(char str){
    static int count = 0;                 
    static int start = 0;
    static int flag = 0;
    static char array[lenstr];
    static int decimal = 0;
    char error[] = " ERROR!"; 
    char ok[7] = " OK: ";  

    if((str == '>') && (start == 1)){               // Stop bit detection
        if((count == 0) || (flag == 1))             // Exclude anything other than 0 - 100 and <>
            mySendString(error);                    // Send error
        else if (count > 0){                        // Start converting chars to decimal
        decimal = strtol(array, NULL, 10);          // Convert string to int using stdlib func      
            if((decimal < 0) || (decimal > 100))    // If decimals are out of range
            mySendString(error);                    // Send error
            else{                                   // Else if decimals are in range
            dutycycle(decimal);                     // Send decimal to PWM
            strcat(ok, array);                      // Join two strings for printing
            mySendString(ok);                       // Send OK
            }
        }
        memset(array, 0, lenstr);                   // Completely clear string
        start = 0;                                  // Reset start
        count = 0;                                  // Reset count
    }    
    if(start == 1){                                 // Continuously read until end bit '<' received
        array[count] = str;                         // Assign received char to array
        if((array[count] <'0') || (array[count] >'9') && (array[count] != '>')){
           flag = 1;                                // Set flag if unwanted char appears in string
        }
        count++;                                    // Increment counter
    }
    if((str == '<') && (start == 0)){               // Continuously read for start bit '<'
        start = 1;                                  // Set the start
        count = 0;                                  // Clear the counter
        flag = 0;                                   // Clear the flag
    }
    if(count == lenstr)                             // If string is longer than max of 100
        count = 0;                                  // Clear counter
}

/***** INTERRUPT SERVICE ROUTINES *****/

/********************************************************
 * Description                                          
 *      Name      : UART2                                                 
 *      Author(s) : O.Nemeth                                                  
 *      ISR       : Stores the received char from U2RXREG
 *                  and send it for decoding             
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018                          
 *      Rev       : 1.0                                 
 *      Alt A     :                                     
 ********************************************************/
void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void){
    IFS1bits.U2RXIF = 0;        // Clear the UART Receive Interrupt Flag
    char str;                   // Declare character 
    
    while(U2STAbits.URXDA)      // Data arrival - while there is data to receive
        str = U2RXREG;          // Read one char at a time from the U2 RX register  
    decode(str);                // Call function to decode char
}

/********************************************************
 * Description                                          
 *      Name      : UART2                                                 
 *      Author(s) : O.Nemeth                                                  
 *      ISR       :              
 ********************************************************
 * Change History                                       
 *      Released  : 22-11-2018                          
 *      Rev       : 1.0                                 
 *      Alt A     :                                     
 ********************************************************/
void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void){
    IFS1bits.U2TXIF= 0;         // clear TX interrupt flag
}


