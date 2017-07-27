/*
 * Copyright (C) 2017 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <map>

#include "ignition/common/Plugin.hh"
#include "ignition/common/PluginInfo.hh"
#include "PluginUtils.hh"

namespace ignition
{
  namespace common
  {
    class PluginPrivate
    {
      /// \brief Map from interface names to their locations within the plugin
      /// instance
      //
      // Dev Note (MXG): We use std::map here instead of std::unordered_map
      // because iterators to a std::map are not invalidated by the insertion
      // operation. This allows us to do optimizations with template magic to
      // provide direct access to interfaces whose availability we can
      // anticipate at run time.
      //
      // It is also worth noting that ordered vs unordered lookup time is very
      // similar on strings with relatively small lengths (5-20 characters) and
      // a relatively small set of strings (5-20 entries), and those conditions
      // match our expected use case here. In fact, ordered lookup can sometimes
      // outperform unordered in these conditions.
      public: std::map<std::string, void*> interfaces;

      /// \brief ptr which manages the lifecycle of the plugin instance. Note
      /// that we cannot use a std::unique_ptr<void> here because unique_ptr
      /// is statically constrained to only hold onto a complete type, which
      /// "void" never qualifies as.
      public: void *pluginInstance;

      /// \brief Function which can be invoked to delete our plugin instance
      public: std::function<void(void*)> pluginDeleter;

      /// \brief Destructor
      public: ~PluginPrivate()
      {
        pluginDeleter(pluginInstance);
      }
    };

    Plugin::Plugin(const PluginInfo &info)
      : dataPtr(new PluginPrivate)
    {
      // Create a new instance of the plugin, and store it in our dataPtr
      this->dataPtr->pluginInstance = info.factory();

      // Grab a copy of the plugin's deleter
      this->dataPtr->pluginDeleter = info.deleter;

      void * const instance = this->dataPtr->pluginInstance;
      // For each interface provided by the plugin, insert its location within
      // the instance into the map
      for(const auto &entry : info.interfaces)
      {
        // entry.first: name of the interface
        // entry.second: function which casts the pluginInstance pointer to the
        //               correct location of the interface within the plugin
        this->dataPtr->interfaces.insert(
              std::make_pair(entry.first, entry.second(instance)));
      }
    }

    Plugin::~Plugin()
    {
      delete dataPtr;
    }

    void *Plugin::GetInterface(const std::string &_interfaceName)
    {
      const std::string interfaceName = NormalizeName(_interfaceName);
      const auto &it = this->dataPtr->interfaces.find(interfaceName);
      if(this->dataPtr->interfaces.end() == it)
        return nullptr;

      return it->second;
    }
  }
}