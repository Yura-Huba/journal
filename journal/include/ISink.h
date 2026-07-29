#pragma once

#include <string>
#include <memory>

namespace journal {

/// @brief Интерфейс вывода.
/// @details Принимает уже отформатированную строку и записывает её в sink.
class ISink {
public:
	virtual ~ISink() = default;

	/// @brief Записывает отформатированное сообщение.
	virtual void write(const std::string& formattedMessage) = 0;

	/// @brief Проверяет, жив ли sink.
	/// @return true, если sink жив.
	virtual bool isValid() const { return true; }
};

using ISinkUniquePtr = std::unique_ptr<ISink>;

}