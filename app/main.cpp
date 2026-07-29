#include "ConsoleApplication.h"
#include "../journal/include/JournalFactory.h"

#include <iostream>
#include <cstdlib>
#include <csignal>
#include <atomic>

// Флаг для обработки сигналов
std::atomic<bool> shutdownFlag {false};

void signalHandler([[maybe_unused]] int signum) {
	shutdownFlag.store(true, std::memory_order_relaxed);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Использование: " << argv[0] << " <файл_журнала> <INFO|ALERT|FAULT>\n";
		return EXIT_FAILURE;
	}

	try {

		// Регистрация обработчиков сигналов (Ctrl+C, kill)
		if (std::signal(SIGINT, signalHandler) == SIG_ERR || std::signal(SIGTERM, signalHandler) == SIG_ERR) {
			throw std::runtime_error("main: не удалось зарегистрировать обработчик сигнала");
		}

		auto importance = journal::fromString(argv[2]);

		if (!importance) {
			throw std::invalid_argument(
				"main: неизвестный уровень важности: '" + std::string(argv[2]) + "'"
			);
		}

		journal::Importance minImportance = *importance;

		journal::JournalConfig config;
		config.filePath = argv[1];
		config.minImportance = minImportance;

		// Создаём асинхронный журнал через фабрику.
		auto asyncJournal = journal::createAsyncRotatingFileJournal(config);

		app::ConsoleApplication app(asyncJournal);

		int exitCode = app.run(shutdownFlag);
		return exitCode;

	} catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}