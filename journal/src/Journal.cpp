#include "../include/Journal.h"

namespace journal {

Journal::Journal(ISinkUniquePtr sink, IFormatterUniquePtr formatter, Importance minImportance)
	: sink_(std::move(sink))
	, formatter_(std::move(formatter))
	, minImportance_(minImportance) {

	if (!sink_) {
		throw std::invalid_argument("Journal: sink не может быть null");
	}
	if (!formatter_) {
		throw std::invalid_argument("Journal: formatter не может быть null");
	}
}

void Journal::write(const std::string& message,
					Importance importance,
					std::time_t timestamp
) {
	// Фильтруем сообщения ниже текущего уровня важности.
	if (importance < getMinImportance()) {
		return;
	}

	std::string formatted = formatter_->format(message, importance, timestamp);
	sink_->write(formatted);
}

void Journal::setMinImportance(Importance importance) {
	minImportance_.store(importance, std::memory_order_release);
}

Importance Journal::getMinImportance() const {
	return minImportance_.load(std::memory_order_acquire);
}

bool Journal::isSinkValid() const {
	return sink_ && sink_->isValid();
}

}