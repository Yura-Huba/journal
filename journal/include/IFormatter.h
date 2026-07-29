#pragma once

#include "IJournal.h"

#include <string>
#include <ctime>

namespace journal {

/// @brief Интерфейс форматтера.
/// @details Преобразует сообщение, уровень важности и время в строку.
class IFormatter {
public:
	virtual ~IFormatter() = default;

	/// @brief Форматирует сообщение.
	virtual std::string format(const std::string& message,
							   Importance importance,
							   std::time_t timestamp) const = 0;
};

using IFormatterUniquePtr = std::unique_ptr<IFormatter>;

}