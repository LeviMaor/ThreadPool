#ifndef WAITABLE_QUEUE_PQ_WRAPPER
#define WAITABLE_QUEUE_PQ_WRAPPER

#include <vector>           // std::vector
#include <queue>            // std::priority_queue
#include <functional>       // std::less

namespace levi
{
    
template<class T, class CONTAINER = std::vector<T>, class COMPARE = std::less<typename CONTAINER::value_type>>
class PQWrapper: private std::priority_queue<T, CONTAINER, COMPARE>
{
public:
    PQWrapper() = default;
    PQWrapper(const PQWrapper& other_) = delete;
    PQWrapper(const PQWrapper&& other_) = delete;
    PQWrapper& operator=(PQWrapper& other_) = delete;
    PQWrapper& operator=(PQWrapper&& other_) = delete;
    ~PQWrapper() = default;

    const T& front() const;

    using std::priority_queue<T, CONTAINER, COMPARE>::push;
    using std::priority_queue<T, CONTAINER, COMPARE>::pop;
    using std::priority_queue<T, CONTAINER, COMPARE>::empty;
    
};

template <class T, class CONTAINER, class COMPARE>
inline const T &PQWrapper<T, CONTAINER, COMPARE>::front() const
{
    return this->top();
}

} // levi

#endif // ILRD_RD147_WAITABLE_QUEUE_PQ_WRAPPER
