#pragma once

#include "IFormatter.h"

namespace journal {

/// @brief Текстовый форматтер.
/// @details Формат по умолчанию:
///          [timestamp] [LEVEL] message
class SimpleFormatter : public IFormatter {
public:
	explicit SimpleFormatter(std::string timestampFormat = "%d.%m.%Y %H:%M:%S");

	/// @brief Форматирует сообщение.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	std::string format(const std::string& message, Importance importance, std::time_t timestamp) const override;

private:
	std::string timestampFormat_;
};

}