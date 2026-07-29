#include "../include/AsyncJournal.h"

#include <iostream>
#include <string>

namespace journal {

AsyncJournal::AsyncJournal(IJournalSharedPtr inner,
						   std::size_t maxQueueSize,
						   ErrorHandler errorHandler)
	: inner_(std::move(inner))
	, maxQueueSize_(maxQueueSize)
	, errorHandler_(std::move(errorHandler)) {
	if (!inner_) {
		throw std::invalid_argument("AsyncJournal: журнал не может быть null");
	}

	if (maxQueueSize_ == 0) {
		throw std::invalid_argument("AsyncJournal: размер очереди должен быть больше нуля");
	}

	// Кэшируем текущее значение уровня важности.
	cachedMinImportance_.store(inner_->getMinImportance(),
							   std::memory_order_release);

	// В конце конструктора, так что не опасно
	workerThread_ = std::thread(&AsyncJournal::worker, this);
}

AsyncJournal::~AsyncJournal() noexcept {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		stop_ = true;
	}

	// Будим рабочий поток, чтобы он увидел флаг остановки.
	cv_.notify_one();

	if (workerThread_.joinable()) {
		try {
			// Ждём обработки всех записей в очереди.
			workerThread_.join();
		} catch (const std::system_error& e) {
			reportError("~AsyncJournal: не удалось выполнить join: " + std::string(e.what()));
		}
	}
}

void AsyncJournal::write(const std::string& message,
						 Importance importance,
						 std::time_t timestamp) {
	bool needLogOverflow = false;
	std::size_t currentDropped = 0;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (stop_) {
			throw std::logic_error("AsyncJournal: команда не помещена в очередь, AsyncJournal завершает работу");
		}

		if (numberOfMessage_ >= maxQueueSize_) {
			++numberOfDroppedCommands_;

			// Логируем не каждое переполнение
			if (numberOfDroppedCommands_ == 1 || numberOfDroppedCommands_ % 100 == 0) {
				needLogOverflow = true;
				currentDropped = numberOfDroppedCommands_;
			}

			return;
		}

		// Ставим сообщение в очередь
		queue_.push(Command {
			CommandType::WRITE_MESSAGE,
			message,
			importance,
			timestamp});

		++numberOfMessage_;
	}

	if (needLogOverflow) {
		reportError("AsyncJournal: очередь переполнена, пропущено: " + std::to_string(currentDropped));
	}

	cv_.notify_one();
}

void AsyncJournal::setMinImportance(Importance importance) {
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (stop_) {
			throw std::logic_error("AsyncJournal: команда не помещена в очередь, AsyncJournal завершает работу");
		}

		// Ставим команду смены уровня важности в очередь
		queue_.push(Command {
			CommandType::CHANGE_MIN_IMPORTANCE,
			std::string{},
			importance,
			0});

		// Обновляем кэш сразу, чтобы приложения видели новое значение
		cachedMinImportance_.store(importance, std::memory_order_release);
	}

	cv_.notify_one();
}

Importance AsyncJournal::getMinImportance() const {
	return cachedMinImportance_.load(std::memory_order_acquire);
}

std::size_t AsyncJournal::getNumberOfDroppedCommands() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return numberOfDroppedCommands_;
}

bool AsyncJournal::isSinkValid() const {
	return inner_ && inner_->isSinkValid();
}

void AsyncJournal::worker() {
	try {
		while (true) {
			Command command;

			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });

				if (stop_ && queue_.empty()) {
					break;
				}

				command = std::move(queue_.front());
				queue_.pop();

				// Под мьютексом от гонки с главным потоком
				if (command.type == CommandType::WRITE_MESSAGE) {
					--numberOfMessage_;
				}
			}

			try {
				if (command.type == CommandType::WRITE_MESSAGE) {
					inner_->write(command.text,
								  command.importance,
								  command.timestamp);
				} else if (command.type == CommandType::CHANGE_MIN_IMPORTANCE) {
					inner_->setMinImportance(command.importance);
				}
			} catch (const std::exception& e) {
				if (handleWorkerError("AsyncJournal: ошибка записи в журнал: " + std::string(e.what()))) {
					break;
				}
			} catch (...) {
				if (handleWorkerError("AsyncJournal: неизвестная ошибка при обработке команды журнала")) {
					break;
				}
			}
		}
	} catch (...) {
		reportError("AsyncJournal: неизвестная ошибка в рабочем потоке");
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stop_ = true;
		}
	}
}

bool AsyncJournal::handleWorkerError(std::string_view message) {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		++numberOfDroppedCommands_;
	}

	reportError(message);

	if (!inner_->isSinkValid()) {
		std::size_t lostCommands = 0;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			lostCommands = queue_.size();
			numberOfDroppedCommands_ += lostCommands;
			stop_ = true;
		}
		reportError("AsyncJournal: Sink невосстановим, рабочий поток останавливается. Потеряно команд: "
					+ std::to_string(lostCommands));
		return true;
	}

	return false;
}

void AsyncJournal::reportError(std::string_view message) const {
	if (errorHandler_) {
		errorHandler_(message);
	} else {
		std::string line(message);
		line += '\n';
		std::cerr << line;
	}
}

}