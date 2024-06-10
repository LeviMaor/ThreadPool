#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <functional>
#include <set>

#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define RESET   "\033[0m"

#include "thread_pool.hpp"

template<typename T>
static std::string Str(const T& d)
{
    std::ostringstream oss;
    oss << d;
    return oss.str();
}

using std::string;
struct Error
{
    Error(string err_, string exp_, string res_, int line_,
    ssize_t iteration_ = -1) :
        err(err_), exp(exp_), res(res_), line(line_),
            iteration(iteration_) { }
    
    void Display()
    {
        std::cout << RED << "\nError : " << err << ", expected : " << exp <<
        ", result : " << res << " , line : " << line;
    
        if (-1 != iteration)
        {
            std::cout << ", iteration : " << iteration;
        }
        
        std::cout << RESET << "\n\n";

    }

    std::string err;
    std::string exp;
    std::string res;
    int line;
    ssize_t iteration;
};


using namespace levi;



enum TEST_PRIORITY
{
    LOW = 1,
    NORMAL = 2,
    HIGH = 3 
};

std::vector<TEST_PRIORITY> testVector;
std::mutex MUTEX;


class LowPriorityTask : public ThreadPool::ITask
{
public:
    virtual void Execute()
    {
        std::unique_lock<std::mutex> lock(MUTEX);
        testVector.push_back(TEST_PRIORITY::LOW);
    }
};




class NormalPriorityTask : public ThreadPool::ITask
{
public:
    virtual void Execute()
    {
        std::unique_lock<std::mutex> lock(MUTEX);
        testVector.push_back(TEST_PRIORITY::NORMAL);
    }
};




class HighPriorityTask : public ThreadPool::ITask
{
public:
    virtual void Execute()
    {
        std::unique_lock<std::mutex> lock(MUTEX);
        testVector.push_back(TEST_PRIORITY::HIGH);
    }
};




class SleepTask : public ThreadPool::ITask
{
public:
    SleepTask(std::chrono::seconds time_) : m_time(time_) { }

    virtual void Execute()
    {
        std::this_thread::sleep_for(m_time);
    }
private:
    std::chrono::seconds m_time; 
};




int main()
{
    const size_t TESTS = 100; // set here the number of loop you want to go through that test 
    for (size_t testNum = 0; testNum < TESTS; ++testNum)
    {

    {
    ThreadPool pool(5);
    pool.Pause(); // load tasks but don't run them

    const size_t LOW_TASKS = 100;
    for (size_t i = 0; i < LOW_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<LowPriorityTask>(), ThreadPool::LOW);
    }

    const size_t NORMAL_TASKS = 100;
    for (size_t i = 0; i < NORMAL_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<NormalPriorityTask>(), ThreadPool::NORMAL);
    }

    const size_t HIGH_TASKS = 100;
    for (size_t i = 0; i < HIGH_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<HighPriorityTask>(), ThreadPool::HIGH);
    }

    pool.Resume(); // run pool

    // Let test vector fill up
    const size_t EXPECTED_VECTOR_SIZE = LOW_TASKS + NORMAL_TASKS + HIGH_TASKS;
    bool wait = true;
    while (true == wait)
    {
        std::unique_lock<std::mutex> mutex(MUTEX);
        if (testVector.size() >= EXPECTED_VECTOR_SIZE)
        {
            wait = false;
        }
    }




    // test results
    try
    {
    // fist 100 should be HIGH PRIORITY tasks results
    size_t i = 0;
    size_t mutexSafety = 5; // becuase of tests mutex use, safety gap is required
    const size_t ALLOWED_ERRORS = 5; 
    size_t errors = 0;    
    for(i = 0;i < HIGH_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::HIGH != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like high priorities tasks were not run firsts",
                        Str(HIGH), Str(testVector[i]), __LINE__, i);
        }
    }

    // 100 - 200 should be NORMAL PRIORITY results
    for(i = HIGH_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::NORMAL != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like normal priorities tasks were not run seconds",
                        Str(NORMAL), Str(testVector[i]), __LINE__, i);
        }
    }

    // 200 - 300 should be LOW PRIORITY results
    for(i = HIGH_TASKS + NORMAL_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS + LOW_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::LOW != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like low priorities tasks were not run lasts",
                        Str(LOW), Str(testVector[i]), __LINE__, i);
        }
    }

    std::cout << GREEN << "Pool of 5 threads passed all priorities tests" << RESET << std::endl;
    }
    catch(Error &e)
    {
        e.Display();
        return -1;
    }
    }








    {
    testVector.clear();
    ThreadPool pool(1);
    pool.Pause(); // load tasks but don't run them

    const size_t LOW_TASKS = 100;
    for (size_t i = 0; i < LOW_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<LowPriorityTask>(), ThreadPool::LOW);
    }

    const size_t NORMAL_TASKS = 100;
    for (size_t i = 0; i < NORMAL_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<NormalPriorityTask>(), ThreadPool::NORMAL);
    }

    const size_t HIGH_TASKS = 100;
    for (size_t i = 0; i < HIGH_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<HighPriorityTask>(), ThreadPool::HIGH);
    }

    pool.Resume(); // run pool
    




    // Let test vector fill up
    const size_t EXPECTED_VECTOR_SIZE = LOW_TASKS + NORMAL_TASKS + HIGH_TASKS;
    bool wait = true;
    while (true == wait)
    {
        std::unique_lock<std::mutex> mutex(MUTEX);
        if (testVector.size() >= EXPECTED_VECTOR_SIZE)
        {
            wait = false;
        }
        mutex.unlock();
    }





    // test results
    try
    {
    // fist 100 should be HIGH PRIORITY tasks results
    size_t i = 0;
    size_t mutexSafety = 5; // becuase of tests mutex use, safety gap is required
    const size_t ALLOWED_ERRORS = 5; 
    size_t errors = 0;    
    for(i = 0;i < HIGH_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::HIGH != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like high priorities tasks were not run firsts",
                        Str(HIGH), Str(testVector[i]), __LINE__, i);
        }
    }

    // 100 - 200 should be NORMAL PRIORITY results
    for(i = HIGH_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::NORMAL != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like normal priorities tasks were not run seconds",
                        Str(NORMAL), Str(testVector[i]), __LINE__, i);
        }
    }

    // 200 - 300 should be LOW PRIORITY results
    for(i = HIGH_TASKS +NORMAL_TASKS +  mutexSafety; i < HIGH_TASKS + NORMAL_TASKS + LOW_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::LOW != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like low priorities tasks were not run lasts",
                        Str(LOW), Str(testVector[i]), __LINE__, i);
        }
    }

    std::cout << GREEN << "Pool of 1 threads passed all priorities tests" << RESET << std::endl;
    }
    catch(Error &e)
    {
        e.Display();
        return -1;
    }
    }








    // Test wether pool goes crazy with multiple pauses and resumes
    {
    testVector.clear();
    ThreadPool pool(8);
    pool.Pause(); // load tasks but don't run them

    const size_t LOW_TASKS = 100;
    for (size_t i = 0; i < LOW_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<LowPriorityTask>(), ThreadPool::LOW);
        pool.Pause(); // Just for fun  
    }

    const size_t NORMAL_TASKS = 100;
    for (size_t i = 0; i < NORMAL_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<NormalPriorityTask>(), ThreadPool::NORMAL);
        pool.Pause();        
    }

    const size_t HIGH_TASKS = 100;
    for (size_t i = 0; i < HIGH_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<HighPriorityTask>(), ThreadPool::HIGH);
        pool.Pause();        
    }

    // Mess with pool's mental health
    for (size_t i = 0, mentalHealthLimit = 500; i < mentalHealthLimit; ++i)
    {
        pool.Pause();
        pool.Resume();
    }




    // Let test vector fill up
    const size_t EXPECTED_VECTOR_SIZE = LOW_TASKS + NORMAL_TASKS + HIGH_TASKS;
    bool wait = true;
    while (true == wait)
    {
        std::unique_lock<std::mutex> mutex(MUTEX);
        if (testVector.size() >= EXPECTED_VECTOR_SIZE)
        {
            wait = false;
        }
        mutex.unlock();
    }





    // test results
    try
    {
    // fist 100 should be HIGH PRIORITY tasks results
    // because of mutexes, not possible to gurentee order 
    const size_t ALLOWED_ERRORS = 5; 
    size_t errors = 0;
    size_t i = 0;
    size_t mutexSafety = 5; // becuase of tests mutex use, safety gap is required
    for(i = 0;i < HIGH_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::HIGH != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like high priorities tasks were not run firsts",
                        Str(HIGH), Str(testVector[i]), __LINE__, i);
        }
    }

    // 100 - 200 should be NORMAL PRIORITY results
    for(i = HIGH_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::NORMAL != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like normal priorities tasks were not run seconds",
                        Str(NORMAL), Str(testVector[i]), __LINE__, i);
        }
    }

    // 200 - 300 should be LOW PRIORITY results
    for(i = HIGH_TASKS + NORMAL_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS + LOW_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::LOW != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like low priorities tasks were not run lasts",
                        Str(LOW), Str(testVector[i]), __LINE__, i);
        }
    }


    std::cout << GREEN << "Pool of 8 threads passed all pause-resume (mental health) tests" << RESET << std::endl;
    }
    catch(Error &e)
    {
        e.Display();
        return -1;
    }
    }




    // Exactly same test with 11-threads pool
    {
    testVector.clear();
    ThreadPool pool(11);
    pool.Pause(); // load tasks but don't run them

    const size_t LOW_TASKS = 100;
    for (size_t i = 0; i < LOW_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<LowPriorityTask>(), ThreadPool::LOW);
        pool.Pause(); // Just for fun  
    }

    const size_t NORMAL_TASKS = 100;
    for (size_t i = 0; i < NORMAL_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<NormalPriorityTask>(), ThreadPool::NORMAL);
        pool.Pause();        
    }

    const size_t HIGH_TASKS = 100;
    for (size_t i = 0; i < HIGH_TASKS; ++i)
    {
        pool.AddTask(std::make_shared<HighPriorityTask>(), ThreadPool::HIGH);
        pool.Pause();        
    }

    // Mess with pool's mental health
    for (size_t i = 0, mentalHealthLimit = 500; i < mentalHealthLimit; ++i)
    {
        pool.Pause();
        pool.Resume();
    }






    // Let test vector fill up
    const size_t EXPECTED_VECTOR_SIZE = LOW_TASKS + NORMAL_TASKS + HIGH_TASKS;
    bool wait = true;
    while (true == wait)
    {
        std::unique_lock<std::mutex> mutex(MUTEX);
        if (testVector.size() >= EXPECTED_VECTOR_SIZE)
        {
            wait = false;
        }
        mutex.unlock();
    }






    // test results
    try
    {
    // fist 100 should be HIGH PRIORITY tasks results
    size_t i = 0;
    const size_t ALLOWED_ERRORS = 5; 
    size_t errors = 0;
    size_t mutexSafety = 5; // becuase of tests mutex use, safety gap is required
    for(i = 0;i < HIGH_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::HIGH != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like high priorities tasks were not run firsts",
                        Str(HIGH), Str(testVector[i]), __LINE__, i);
        }
    }

    // 100 - 200 should be NORMAL PRIORITY results
    for(i = HIGH_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::NORMAL != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like normal priorities tasks were not run seconds",
                        Str(NORMAL), Str(testVector[i]), __LINE__, i);
        }
    }

    // 200 - 300 should be LOW PRIORITY results
    for(i = HIGH_TASKS + NORMAL_TASKS + mutexSafety; i < HIGH_TASKS + NORMAL_TASKS + LOW_TASKS - mutexSafety; ++i)
    {
        if (TEST_PRIORITY::LOW != testVector[i] && ++errors >= ALLOWED_ERRORS)
        {
            throw Error("Seems like low priorities tasks were not run lasts",
                        Str(LOW), Str(testVector[i]), __LINE__, i);
        }
    }


    std::cout << GREEN << "Pool of 11 threads passed all pause-resume (mental health) tests" << RESET << std::endl;
    }
    catch(Error &e)
    {
        e.Display();
        return -1;
    }
    }
    testVector.clear();



    }


    std::cout << GREEN << "All Good!" << RESET << std::endl;
	return 0;
}
