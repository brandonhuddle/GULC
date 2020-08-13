/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "Linker.hpp"

void gulc::Linker::link(std::vector<ObjFile>& objFiles) {
    // Create the entry object...
    std::string asmArgs = "as examples/entry.x64.s -o build/objs/examples/entry.o";
    std::system(asmArgs.c_str());

    std::string objFilesPath;

    for (ObjFile& objFile : objFiles) {
        objFilesPath += " " + objFile.filePath;
    }

    // Link everything together...
    std::string linkerArgs = "ld build/objs/examples/entry.o " + objFilesPath + " -o a.out";
    std::system(linkerArgs.c_str());
}
