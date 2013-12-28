
#include <random>
#include <iostream>
#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <sys/ioctl.h>

#include <linux/serial.h>

#include <csignal>
#include <cstdint>
#include <bitset>
#include <vector>

#include <pthread.h>
#include <sched.h>

void set_realtime_priority() {
	int ret;

	// We'll operate on the currently running thread.
	pthread_t this_thread = pthread_self();
	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	std::cout << "Trying to set thread realtime prio = " << params.sched_priority << std::endl;

	// Attempt to set thread real-time priority to the SCHED_FIFO policy
	ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
	if (ret != 0) {
		// Print the error
		std::cout << "Unsuccessful in setting thread realtime prio (erno=" << ret << ")" << std::endl;
		return;
	}

	// Now verify the change in thread priority
	int policy = 0;
	ret = pthread_getschedparam(this_thread, &policy, &params);
	if (ret != 0) {
		std::cout << "Couldn't retrieve real-time scheduling paramers" << std::endl;
		return;
	}

	// Check the correct policy was applied
	if(policy != SCHED_FIFO) {
		std::cout << "Scheduling is NOT SCHED_FIFO!" << std::endl;
	} else {
		std::cout << "SCHED_FIFO OK" << std::endl;
	}

	// Print thread scheduling priority
	std::cout << "Thread priority is " << params.sched_priority << std::endl;
}


struct ColorSignal
{
	uint8_t green_1;
	uint8_t green_2;
	uint8_t green_3;
	uint8_t green_4;

	uint8_t red_1;
	uint8_t red_2;
	uint8_t red_3;
	uint8_t red_4;

	uint8_t blue_1;
	uint8_t blue_2;
	uint8_t blue_3;
	uint8_t blue_4;
};

static ColorSignal RED_Signal = { 0xCE, 0xCE, 0xCE, 0xCE,
								  0xCE, 0x8C, 0x8C, 0x8C,
								  0xCE, 0xCE, 0xCE, 0xCE };
static ColorSignal GREEN_Signal = { 0xCE, 0x8C, 0x8C, 0x8C,
									0xCE, 0xCE, 0xCE, 0xCE,
									0xCE, 0xCE, 0xCE, 0xCE };
static ColorSignal BLUE_Signal = { 0xCE, 0xCE, 0xCE, 0xCE,
								   0xCE, 0xCE, 0xCE, 0xCE,
								   0xCE, 0x8C, 0x8C, 0x8C};
static ColorSignal BLACK_Signal = { 0xCE, 0xCE, 0xCE, 0xCE,
								   0xCE, 0xCE, 0xCE, 0xCE,
								   0xCE, 0xCE, 0xCE, 0xCE};

static volatile bool _running;

void signal_handler(int signum)
{
	_running = false;

}

int main()
{
	_running = true;
	signal(SIGTERM, &signal_handler);

	//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream = -1;

	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyAMA0", O_WRONLY | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

//	if (0)
	{
		//CONFIGURE THE UART
		//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
		//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
		//	CSIZE:- CS5, CS6, CS7, CS8
		//	CLOCAL - Ignore modem status lines
		//	CREAD - Enable receiver
		//	IGNPAR = Ignore characters with parity errors
		//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
		//	PARENB - Parity enable
		//	PARODD - Odd parity (else even)
		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B4000000 | CS8 | CLOCAL;		//<Set baud rate
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		cfmakeraw(&options);

		std::cout << "options.c_cflag = " << options.c_cflag << std::endl;
		std::cout << "options.c_iflag = " << options.c_iflag << std::endl;
		std::cout << "options.c_oflag = " << options.c_oflag << std::endl;
		std::cout << "options.c_lflag = " << options.c_lflag << std::endl;

		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);
		// Let's verify configured options
		tcgetattr(uart0_filestream, &options);

		std::cout << "options.c_cflag = " << options.c_cflag << std::endl;
		std::cout << "options.c_iflag = " << options.c_iflag << std::endl;
		std::cout << "options.c_oflag = " << options.c_oflag << std::endl;
		std::cout << "options.c_lflag = " << options.c_lflag << std::endl;
	}
	{
		struct serial_struct ser;

		if (-1 == ioctl(uart0_filestream, TIOCGSERIAL, &ser))
		{
			std::cerr << "Failed to obtian 'serial_struct' for setting custom baudrate" << std::endl;
		}

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 4000000" << std::endl;

		// set custom divisor
		ser.custom_divisor = ser.baud_base / 8000000;
		// update flags
		ser.flags &= ~ASYNC_SPD_MASK;
		ser.flags |= ASYNC_SPD_CUST;

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 8000000" << std::endl;


		if (-1 == ioctl(uart0_filestream, TIOCSSERIAL, &ser))
		{
			std::cerr << "Failed to configure 'serial_struct' for setting custom baudrate" << std::endl;
		}

		// Check result
		if (-1 == ioctl(uart0_filestream, TIOCGSERIAL, &ser))
		{
			std::cerr << "Failed to obtian 'serial_struct' for setting custom baudrate" << std::endl;
		}

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 4000000" << std::endl;
	}


	if (uart0_filestream < 0)
	{
		std::cerr << "Opening the device has failed" << std::endl;
		return -1;
	}

	//----- TX BYTES -----
	std::vector<ColorSignal> signalData(10, RED_Signal);

	int loopCnt = 0;
	std::cout << "Type 'c' to continue, 'q' or 'x' to quit: ";
	while (_running)
	{
		char c = getchar();
		if (c == 'q' || c == 'x')
		{
			break;
		}
		if (c != 'c')
		{
			continue;
		}

		set_realtime_priority();
		for (int iRun=0; iRun<10; ++iRun)
		{
//			tcflush(uart0_filestream, TCOFLUSH);
			write(uart0_filestream, signalData.data(), signalData.size()*sizeof(ColorSignal));
			tcdrain(uart0_filestream);

			usleep(100000);
			++loopCnt;

			if (loopCnt%3 == 2)
				signalData = std::vector<ColorSignal>(10, GREEN_Signal);
			else if(loopCnt%3 == 1)
				signalData = std::vector<ColorSignal>(10, BLUE_Signal);
			else if(loopCnt%3 == 0)
				signalData = std::vector<ColorSignal>(10, RED_Signal);

		}
	}

	signalData = std::vector<ColorSignal>(50, BLACK_Signal);
	write(uart0_filestream, signalData.data(), signalData.size()*sizeof(ColorSignal));
	//----- CLOSE THE UART -----
	close(uart0_filestream);

	std::cout << "Program finished" << std::endl;

	return 0;
}