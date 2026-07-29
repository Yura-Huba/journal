#pragma once

#include "IJournal.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>

namespace journal {

/// @brief Асинхронный декоратор IJournal.
/// @details Принимает сообщения и команды смены уровня важности,
///          помещает их в очередь и обрабатывает в отдельном потоке.
///          Порядок команд сохраняется.
/// @note При переполнении очереди сообщения отбрасываются.
class AsyncJournal : public IJournal {
public:
	using IJournal::write;

	/// @brief Создаёт асинхронный журнал.
	/// @throw std::invalid_argument если inner = nullptr или maxQueueSize = 0.
	explicit AsyncJournal(IJournalSharedPtr inner, std::size_t maxQueueSize = 10000, ErrorHandler errorHandler = {});

	AsyncJournal(const AsyncJournal&) = delete;
	AsyncJournal& operator=(const AsyncJournal&) = delete;
	AsyncJournal(AsyncJournal&&) = delete;
	AsyncJournal& operator=(AsyncJournal&&) = delete;

	/// @brief Останавливает рабочий поток и ждёт завершения обработки очереди.
	~AsyncJournal() noexcept override;

	/// @brief Помещает сообщение в очередь.
	/// @throw std::logic_error если журнал останавливается.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	void write(const std::string& message,
			   Importance importance,
			   std::time_t timestamp) override;

	/// @brief Помещает команду смены уровня важности в очередь.
	/// @throw std::logic_error если журнал останавливается.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	void setMinImportance(Importance importance) override;

	/// @brief Возвращает кэшированный уровень важности.
	/// @return текущий уровень важности.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	Importance getMinImportance() const override;

	/// @brief Возвращает число отброшенных команд.
	/// @return число отброшенных команд.
	/// @par Потокобезопасность
	/// Потокобезопасен.
	std::size_t getNumberOfDroppedCommands() const;

	/// @brief Проверяет, жив ли внутренний sink Journal.
	/// @return true, если внутренний sink Journal жив.
	/// @par Потокобезопасность
	/// Потокобезопасность зависит от Journal.
	bool isSinkValid() const override;

private:
	void worker();
	bool handleWorkerError(std::string_view message);
	void reportError(std::string_view message) const;

	/// @brief Тип команды в очереди.
	enum class CommandType {
		WRITE_MESSAGE,
		CHANGE_MIN_IMPORTANCE
	};

	/// @brief Команда очереди.
	struct Command {
		CommandType type = CommandType::WRITE_MESSAGE;
		std::string text;
		Importance importance = Importance::INFO;
		std::time_t timestamp = 0;
	};

	IJournalSharedPtr inner_;
	std::queue<Command> queue_;

	/// @brief Максимальный размер очереди для сообщений.
	/// @note Команды смены уровня не учитываются, чтобы не потерять.
	std::size_t maxQueueSize_;
	std::size_t numberOfMessage_ = 0;
	std::size_t numberOfDroppedCommands_ = 0;

	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::thread workerThread_;
	bool stop_ = false;

	std::atomic<Importance> cachedMinImportance_;

	ErrorHandler errorHandler_;
};

}