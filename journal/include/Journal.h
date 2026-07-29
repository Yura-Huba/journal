#pragma once

#include "IJournal.h"
#include "ISink.h"
#include "IFormatter.h"

#include <atomic>

namespace journal {

/// @brief Синхронный журнал.
/// @details Фильтрует сообщения по уровню важности,
///          форматирует их и передаёт в sink.
class Journal : public IJournal {
public:
	using IJournal::write;

	/// @brief Создаёт журнал.
	/// @throw std::invalid_argument если sink или formatter = nullptr.
	Journal(ISinkUniquePtr sink, IFormatterUniquePtr formatter, Importance minImportance);

	Journal(const Journal&) = delete;
	Journal& operator=(const Journal&) = delete;
	Journal(Journal&&) = delete;
	Journal& operator=(Journal&&) = delete;

	~Journal() override = default;

	/// @brief Фильтрует, форматирует и записывает сообщение в sink.
	/// @par Потокобезопасность
	/// Потокобезопасен при потокобезопасных sink и formatter.
	void write(const std::string& message,
			   Importance importance,
			   std::time_t timestamp
	) override;

	/// @brief Устанавливает минимальный уровень важности.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	void setMinImportance(Importance importance) override;

	/// @brief Возвращает минимальный уровень важности.
	/// @return текущий уровень важности.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	Importance getMinImportance() const override;

	/// @brief Проверяет, жив ли sink.
	/// @return true, если sink существует и жив.
	/// @par Потокобезопасность
	/// Потокобезопасность зависит от sink.
	bool isSinkValid() const override;
private:
	ISinkUniquePtr sink_;
	IFormatterUniquePtr formatter_;
	std::atomic<Importance> minImportance_;
};

}