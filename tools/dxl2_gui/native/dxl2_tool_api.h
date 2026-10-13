/**
 * @file dxl2_tool_api.h
 * @brief Symbol export macro of the DXL2 GUI native library.
 *
 * Project: DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
 * Author: Anton Vynohradov
 * Email: avynohradov@systemfromscratch.com
 *
 * @copyright Copyright (c) 2026 Anton Vynohradov
 * @license Apache License 2.0
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
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DXL2_TOOL_API_H
#define DXL2_TOOL_API_H

/**
 * @def DXL2_TOOL_API
 * @brief Marks a function exported from the shared library loaded by the GUI.
 *
 * @details The static DXL2 library is linked into a shared library; only the
 *          functions marked with this macro form its interface, so the Python
 *          side never depends on the layout of the library structures.
 */
#if defined(_WIN32)
#define DXL2_TOOL_API __declspec(dllexport)
#elif defined(__GNUC__)
#define DXL2_TOOL_API __attribute__((visibility("default")))
#else
#define DXL2_TOOL_API
#endif

#endif /* DXL2_TOOL_API_H */
