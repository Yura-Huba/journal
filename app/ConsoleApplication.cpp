#include "ConsoleApplication.h"

#include <cstdlib>

namespace app {

ConsoleApplication::ConsoleApplication(journal::IJournalSharedPtr journal,
									   std::istream& input,
									   std::ostream& output)
	: journal_(std::move(journal))
	, input_(input)
	, output_(output) {
	if (!journal_) {
		throw std::invalid_argument("ConsoleApplication: журнал не может быть null");
	}
}

int ConsoleApplication::run(const std::atomic<bool>& shutdownFlag) {
	output_ << "=== Журнал сообщений ===\n";
	output_ << "Инструкция:\n";
	output_ << "  1. Введите просто текст (он запишется с текущим уровнем '"
		<< journal::toString(journal_->getMinImportance()) << "'):\n";
	output_ << "     > Обычное сообщение\n";
	output_ << "  2. Укажите уровень важности в начале строки через пробел:\n";
	output_ << "     > ALERT Важное событие\n";
	output_ << "  3. Изменить уровень по умолчанию:\n";
	output_ << "     > !set FAULT\n";
	output_ << "  4. Выход: Ctrl+D или Ctrl+C + Enter.\n";
	output_ << "------------------------\n";

	int returnCode = processInput(shutdownFlag);

	output_ << "\nЗавершение работы...\n";
	return returnCode;
}

int ConsoleApplication::processInput(const std::atomic<bool>& shutdownFlag) {
	std::string line;

	while (!shutdownFlag.load(std::memory_order_relaxed)) {
		if (!journal_->isSinkValid()) {
			output_ << "[Система] Ошибка sink журнала.\n";
			return EXIT_FAILURE;
		}

		if (!std::getline(input_, line)) {
			break;
		}

		std::string_view trimmedLine = trim(line);
		if (trimmedLine.empty()) continue;

		// Пытаемся обработать как команду. Если получилось — идем на след. итерацию
		if (handleCommand(trimmedLine)) {
			continue;
		}

		// Если это не команда, обрабатываем как обычное сообщение
		processMessage(trimmedLine);
	}

	return EXIT_SUCCESS;
}

bool ConsoleApplication::handleCommand(std::string_view line) {
	constexpr std::string_view setPrefix = "!set";

	// Если это не команда, возвращаем false
	if (line.length() < setPrefix.length() || line.substr(0, setPrefix.length()) != setPrefix) {
		return false;
	}

	if (line.length() > setPrefix.length()) {
		const char next = line[setPrefix.length()];
		if (next != ' ' && next != '\t') {
			return false;
		}
	}

	std::string_view commandArgs = trim(line.substr(setPrefix.length()));
	if (commandArgs.empty()) {
		output_ << "[Система] Ошибка: укажите уровень важности после " << setPrefix << "\n";
		return true;
	}

	auto newImportance = journal::fromString(commandArgs);

	if (newImportance) {
		try {
			journal_->setMinImportance(*newImportance);
			output_ << "[Система] Уровень важности изменен на: " << commandArgs << "\n";
		} catch (const std::exception& e) {
			output_ << "[Система] Ошибка записи: " << e.what() << "\n";
		}
	} else {
		output_ << "[Система] Неизвестный уровень важности: '"
			<< commandArgs << "', можно: INFO, ALERT, FAULT\n";
	}

	return true;
}

void ConsoleApplication::processMessage(std::string_view line) {
	journal::Importance importance = journal_->getMinImportance();
	std::string_view text = line;

	std::size_t spacePos = line.find(' ');

	if (spacePos != std::string_view::npos) {
		std::string_view firstWord = line.substr(0, spacePos);

		auto parsedImportance = journal::fromString(firstWord);

		if (parsedImportance) {
			importance = *parsedImportance;
			std::string_view unTrimmedText = line.substr(spacePos + 1);
			text = trim(unTrimmedText);
		}
	}

	if (text.empty()) {
		output_ << "[Система] Пропущена пустая запись\n";
		return;
	}

	try {
		journal_->write(std::string(text), importance);
	} catch (const std::exception& e) {
		output_ << "[Система] Ошибка записи: " << e.what() << "\n";
	}
}


std::string_view ConsoleApplication::trim(std::string_view string) {
	std::size_t start = string.find_first_not_of(" \t\r\n");
	if (start == std::string_view::npos) return {};

	std::size_t end = string.find_last_not_of(" \t\r\n");
	return string.substr(start, end - start + 1);
}

}