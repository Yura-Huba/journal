#pragma once

#include "../journal/include/IJournal.h"

#include <iostream>
#include <atomic>
#include <string_view>

namespace app {

/// @brief Консольное приложение для записи сообщений в журнал.
/// @details Читает строки из входного потока.
///          Поддерживает команду !set и указание уровня важности
///          в начале строки.
class ConsoleApplication {
public:
	/// @brief Создание.
	/// @throw std::invalid_argument если journal = nullptr.
	explicit ConsoleApplication(journal::IJournalSharedPtr journal,
					   std::istream& input = std::cin,
					   std::ostream& output = std::cout);

	/// @brief Запускает основной цикл чтения строк.
	/// @return код возврата приложения.
	/// @par Потокобезопасность
	/// Метод не является потокобезопасным.
	int run(const std::atomic<bool>& shutdownFlag);

private:
	// Основной цикл чтения строк.
	int processInput(const std::atomic<bool>& shutdownFlag);

	// Обрабатывает команды (!set).
	// true, если строка команда.
	bool handleCommand(std::string_view line);

	// Разбирает строку как [LEVEL] text и пишет в журнал.
	void processMessage(std::string_view line);

	// Убирает пробельные символы с обоих концов.
	static std::string_view trim(std::string_view string);

	journal::IJournalSharedPtr journal_;
	std::istream& input_;
	std::ostream& output_;
};

}