/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        Simple destructor handler:

        ov_teardown_register(my_destructor);
        ov_teardown_register(my_other_destructor);

        // Do stuff...

        ov_teardown_register(my_intermediate_destructor);
        ov_teardown_register(my_destructor); // again...

        // Do more stuff...

        Upon exiting, do

        ov_teardown();

        That's it.

        The constructor that is registered first, is called last (FIFO):

        ov_teardown_register(my_destructor);
        ov_teardown_register(my_other_destructor);
        ov_teardown_register(my_third_destructor);

        ov_teardown(); // -> calls my_third_destructor, then my_other_destructor,
                       //    and lastly my_destructor

        You can register destructors any time.

        Whether a destructor that is registered several times is called
        several times can be controlled by using either

        ov_teardown_register(destructor) // Only registers the destructor ONCE 
                                         // - the first time it is called

        ov_teardown_register_multiple(destructor); // Registers the destructor
                                                   // multiple times.
                                                   // The destructor is called
                                                   // multiple times - according
                                                   // to the register order.

        ------------------------------------------------------------------------
*/
#ifndef OV_TEARDOWN_H
#define OV_TEARDOWN_H
/*----------------------------------------------------------------------------*/

#include <stdbool.h>

/*----------------------------------------------------------------------------*/

/**
 * Registers a destructor for calling upon teardown.
 * Subsequent calls for the same destructor will not register the destructor
 * again.
 */
bool ov_teardown_register(void (*destructor)(void), char const *name);

/**
 * Registers a destructor for calling upon teardown.
 * Subsequent calls for the same destructor will register the destructor again.
 * If you called `ov_teardown_register_multiple(my_destructor)` thrice,
 * `my_destructor will get called thrice.
 */
bool ov_teardown_register_multiple(void (*destructor)(void), char const *name);

/**
 * Calls all registered destructors in reverse order of their registration.
 */
bool ov_teardown();

/*----------------------------------------------------------------------------*/
#endif
