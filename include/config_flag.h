#pragma once

#if (defined TASKLIB_FLAG_TYPE_SIMPLE)
    using TasklibFlag =         simple_flag;
    #define flag_init
    #define flag_wait(f)        f.wait()
    #define flag_clear(f)       f.clear()
    #define flag_set(f)         f.set()
#elif (defined TASKLIB_FLAG_TYPE_ATOMIC_FLAG) // use atomic_flag
    using TasklibFlag =         atomic_flag;
    #define flag_init           ATOMIC_FLAG_INIT
    #define flag_wait(f)        f.wait(false)
    #define flag_clear(f)       f.clear()
    #define flag_set(f)         { /* NEEDS TO BE ATOMIC */ f.test_and_set(); f.notify_all(); }
#else // use atomic_bool
    using TasklibFlag =         atomic_bool;
    #define flag_init           false
    #define flag_wait(f)        f.wait(true)       
    #define flag_clear(f)       f.store(false)
    #define flag_set(f)         { f.store(true); f.notify_all(); }

#endif
