/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Form.h"

namespace portal {

const std::string *FormResult::error(const Field *field) const
{
    for (const auto &[errorField, text] : errors)
    {
        if (errorField == field)
            return &text;
    }

    return nullptr;
}

FormResult submit(const Fields &fields, const FormArgs &args)
{
    FormResult result;

    for (const auto &field : fields)
    {
        std::string text;
        if (!field->editable() || !args(field->key(), text))
            continue;

        if (!field->stage(text))
            result.errors.emplace_back(field.get(), text);
    }

    for (const auto &field : fields)
    {
        if (result.ok() && field->pending())
        {
            field->commit();
            ++result.changed;
        }
        field->discard();
    }

    return result;
}

} // namespace portal
