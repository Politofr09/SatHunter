#include "Application.h"
#include <exception>
#include <iostream>

int main(void)
{
	SatHunter::Application app = SatHunter::Application();

	try {
		app.Run();
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}