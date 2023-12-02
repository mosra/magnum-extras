#ifndef Magnum_Whee_Implementation_userInterfaceState_h
#define Magnum_Whee_Implementation_userInterfaceState_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/* Definition of the UserInterface::State struct to be used by both
   UserInterface and UserInterfaceGL as well as UserInterface tests, and (if
   this header gets published) eventually possibly also 3rd party renderer
   implementations */

#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee {

struct UserInterface::State {
    /* So that UserInterfaceGL::State and potential other derived state structs
       can be deleted through the base pointer. Same approach is used for
       AbstractVisualLayer::State and derived structs. */
    virtual ~State() = default;

    Containers::Optional<PluginManager::Manager<Text::AbstractFont>> fontManagerStorage;
    PluginManager::Manager<Text::AbstractFont>* fontManager;

    Containers::Optional<PluginManager::Manager<Trade::AbstractImporter>> importerManagerStorage;
    PluginManager::Manager<Trade::AbstractImporter>* importerManager;

    BaseLayer* baseLayer{};
    TextLayer* textLayer{};
    EventLayer* eventLayer{};
};

}}

#endif
