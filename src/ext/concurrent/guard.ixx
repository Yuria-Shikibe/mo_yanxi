//
// Created by Matrix on 2025/11/20.
//

export module mo_yanxi.concurrent.guard;

import std;

namespace mo_yanxi::ccur{

export
template <std::ptrdiff_t I>
struct semaphore_acq_guard{
	std::counting_semaphore<I>& semaphore;

	[[nodiscard]] explicit(false) semaphore_acq_guard(std::counting_semaphore<I>& semaphore) noexcept
		: semaphore(semaphore){
		semaphore.acquire();
	}

	[[nodiscard]] explicit(false) semaphore_acq_guard(std::counting_semaphore<I>& semaphore, std::adopt_lock_t) noexcept
		: semaphore(semaphore){
	}

	~semaphore_acq_guard(){
		semaphore.release();
	}
};


}