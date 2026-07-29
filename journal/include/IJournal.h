#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <optional>
#include <ctime>
#include <functional>
#include <string_view>

namespace journal {

/// @brief Уровень важности сообщения.
/// @note Порядок значений: INFO < ALERT < FAULT.
enum class Importance { INFO = 0, ALERT, FAULT };

/// @brief Возвращает строковое имя уровня важности.
inline std::string toString(Importance importance) {
	switch (importance) {
	case Importance::INFO:  return "INFO";
	case Importance::ALERT: return "ALERT";
	case Importance::FAULT: return "FAULT";
	}
	return "UNKNOWN";
}

/// @brief Разбирает уровень важности из строки.
/// @return уровень важности, иначе std::nullopt.
inline std::optional<Importance> fromString(std::string_view string) noexcept {
	if (string == "INFO")  return Importance::INFO;
	if (string == "ALERT") return Importance::ALERT;
	if (string == "FAULT") return Importance::FAULT;

	return std::nullopt;
}

/// @brief Обработчик внутренних ошибок журнала.
/// @par Потокобезопасность
/// Реализация должна быть потокобезопасной.
using ErrorHandler = std::function<void(std::string_view)>;

/// @brief Интерфейс журнала.
class IJournal {
public:
	virtual ~IJournal() = default;

	/// @brief Записывает сообщение в журнал.
	virtual void write(const std::string& message,
					   Importance importance,
					   std::time_t timestamp) = 0;

	/// @brief Записывает сообщение с текущим временем.
	virtual void write(
		const std::string& message,
		Importance importance
	) final {
		write(message, importance, std::time(nullptr));
	}

	/// @brief Устанавливает минимальный уровень важности.
	virtual void setMinImportance(Importance importance) = 0;

	/// @brief Возвращает минимальный уровень важности.
	/// @return текущий уровень важности.
	virtual Importance getMinImportance() const = 0;

	/// @brief Проверяет, жив ли sink.
	/// @return true, если sink жив.
	virtual bool isSinkValid() const { return true; }
};

using IJournalSharedPtr = std::shared_ptr<IJournal>;

}