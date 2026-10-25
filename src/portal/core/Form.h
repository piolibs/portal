/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Field.h"

namespace portal {

struct FormResult {
    // Fields with a new value
    size_t changed = 0;
    // Invalid fields and their submitted text
    std::vector<std::pair<const Field *, std::string>> errors;

    bool ok() const { return errors.empty(); }
    const std::string *error(const Field *field) const;
};

// Form argument by name, false if it is missing
using FormArgs = std::function<bool(const char *name, std::string &value)>;

// Apply a submitted settings form: all editable fields are validated first,
// a single invalid field rejects the whole form. Only the fields whose value
// changed are stored. Missing and unknown arguments are ignored.
FormResult submit(const Fields &fields, const FormArgs &args);

} // namespace portal
