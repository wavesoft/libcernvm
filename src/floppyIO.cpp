// File:   FloppyIO.cpp
// Author: Ioannis Charalampidis <ioannis.charalampidis AT cern DOT ch>
//
// Hypervisor-Virtual machine bi-directional communication
// through floppy disk.
//
// This class provides the hypervisor-side of the script.
// For the guest-side, check the perl scripts that
// were available with this code.
// 
// Here is the layout of the floppy disk image (Example of 28k):
//
// +-----------------+------------------------------------------------+
// | 0x0000 - 0x37FF |  Hypervisor -> Guest Buffer                    |
// | 0x3800 - 0x6FFE |  Guest -> Hypervisor Buffer                    |
// |     0x6FFF      |  "Data available for guest" flag byte          |
// |     0x7000      |  "Data available for hypervisor" flag byte     |
// +-----------------+------------------------------------------------+
// 
// Created on November 24, 2011, 12:30 PM

#include "floppyIO.h"


// Floppy file constructor
// 
// This constructor opens the specified floppy disk image, fills everything
// with zeroes and initializes the topology variables.
// 
// @param filename The filename of the floppy disk image

FloppyIO::FloppyIO(const char * filename) {
  CRASH_REPORT_BEGIN;
    
  // Open file
  fstream *fIO = new fstream(filename, fstream::in | fstream::out | fstream::trunc | fstream::binary );

  // Prepare floppy info
  this->fIO = fIO;
  this->szFloppy = DEFAULT_FLOPPY_SIZE;

  // Setup offsets and sizes of the I/O parts
  this->szOutput = this->szFloppy/2-1;
  this->ofsOutput = 0;
  this->szInput = this->szOutput;
  this->ofsInput = this->szOutput;
  this->ofsCtrlByteOut = this->szInput+this->szOutput;
  this->ofsCtrlByteIn = this->szInput+this->szOutput+1;

  // Reset floppy file
  this->reset();

  CRASH_REPORT_END;
}

// Advanced Floppy file constructor
// 
// This constructor allows you to open a floppy disk image with extra flags.
// 
// F_NOINIT         Disables the reseting of the image file at open
// F_NOCREATE       Does not truncate the file at open (If not exists, the file will be created)
// F_SYNCHRONIZED   The communication is synchronized, meaning that the code will block until the 
//                  data are read/written from the guest. [NOT YET IMPLEMENTED]
// 
// @param filename The filename of the floppy disk image

FloppyIO::FloppyIO(const char * filename, int flags) {
  CRASH_REPORT_BEGIN;
    
  // Open file
  ios_base::openmode fOpenFlags = fstream::in | fstream::out;
  if ((flags & F_NOCREATE) == 0) fOpenFlags = fstream::trunc;
  fstream *fIO = new fstream(filename, fOpenFlags);
  if (!fIO) {
      this->fIO = NULL;
      return;
  }

  // Check for errors while F_NOCREATE is there
  if ((flags & F_NOCREATE) != 0) {
      if ( (fIO->rdstate() & ifstream::failbit ) != 0 ) {
          
          // Clear error flag
          fIO->clear();
          
          // Try to create file
          fOpenFlags |= fstream::trunc;
          fIO->open(filename, fOpenFlags);
          
          // Still errors?
          if ( (fIO->rdstate() & ifstream::failbit ) != 0 ) {
            cerr << "Error opening '" << filename << "'!\n";
            return;
          }
          
          // Managed to open it? Reset it...
          flags &= ~F_NOINIT;
      }
          
  }
  
  // Prepare floppy info
  this->fIO = fIO;
  this->szFloppy = DEFAULT_FLOPPY_SIZE;
  
  // Setup offsets and sizes of the I/O parts
  this->szOutput = this->szFloppy/2-1;
  this->ofsOutput = 0;
  this->szInput = this->szOutput;
  this->ofsInput = this->szOutput;
  this->ofsCtrlByteOut = this->szInput+this->szOutput;
  this->ofsCtrlByteIn = this->szInput+this->szOutput+1;
  
  // Reset floppy file
  if ((flags & F_NOINIT) == 0) this->reset();

  CRASH_REPORT_END;
}


// FloppyIO Destructor
// Closes the file descriptor and releases used memory

FloppyIO::~FloppyIO() {
    CRASH_REPORT_BEGIN;

    // Make sure we are initialized
    if (this->fIO == NULL) return;

    // Close file
    this->fIO->close();
    
    // Release memory
    delete this->fIO;

    CRASH_REPORT_END;
}

// Reset the floppy disk image
// This function zeroes-out the contents of the FD image
 
void FloppyIO::reset() {
    CRASH_REPORT_BEGIN;
    // Make sure we are initialized
    if (this->fIO == NULL) return;

    // Reset buffers
    this->fIO->seekp(0, ios_base::beg);
    char * buffer = new char[this->szFloppy];
    memset(buffer,0,this->szFloppy);
    this->fIO->write(buffer, this->szFloppy);
    this->fIO->flush();
    delete[] buffer;      
    CRASH_REPORT_END;
}

// Send data to the floppy image I/O
// @param data
// @return 
void FloppyIO::send(string strData) {
    CRASH_REPORT_BEGIN;

    // Make sure we are initialized
    if (this->fIO == NULL) return;

    // Prepare send buffer
    char * dataToSend = new char[this->szOutput];
    memset(dataToSend, 0, this->szOutput);
    
    // Initialize variables
    int szData = strData.length();
    
    // Copy the first szInput bytes
    if (szData > this->szOutput-1) {
        // Data more than the pad size? Trim...
        strData.copy(dataToSend, this->szOutput-1, 0);
    } else {
        // Else, copy the string to send buffer
        strData.copy(dataToSend, szData, 0);
    }
    
    // Write the data to file
    this->fIO->seekp(this->ofsOutput, ios_base::beg);
    this->fIO->write(dataToSend, this->szOutput);
    
    // Notify the client that we placed data (Client should clear this on read)
    this->fIO->seekp(this->ofsCtrlByteOut, ios_base::beg);
    this->fIO->write("\x01", 1);

    // Delete buffer
    delete[] dataToSend;
    CRASH_REPORT_END;
}


// Receive the input buffer contents
// @return Returns a string object with the file contents

string FloppyIO::receive() {
    CRASH_REPORT_BEGIN;
    // Make sure we are initialized
    if (this->fIO == NULL) return "";

    // Prepare buffers
    static string ansBuffer;
    char * dataToReceive = new char[this->szInput];
    
    // Find the size of the input string
    memset( dataToReceive, 0, this->szInput );
    
    // Read the input bytes from FD
    this->fIO->seekg(this->ofsInput, ios_base::beg);
    this->fIO->read(dataToReceive, this->szInput);
    
    // Notify the client that we have read the data
    this->fIO->seekp(this->ofsCtrlByteIn, ios_base::beg);
    this->fIO->write("\x00", 1);
    
    // Copy input data to string object
    ansBuffer = dataToReceive;
    delete[] dataToReceive;

    // Return answer buffer
    return ansBuffer;
    CRASH_REPORT_END;
}