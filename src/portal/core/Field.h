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
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Access.h"

namespace portal {

/*
 * A value shown on the dashboard and, if editable, on the settings page.
 *
 * The value isn't copied: it is read through the getter on every render, so
 * the page always shows the current value. A form value is first staged
 * (parsed and validated) and committed only when the whole form is valid.
 *
 * Strings are not copied either: key, label and unit must outlive the field
 * (string literals).
 */
class Field {
public:
    Field(const char *key, const char *label, const char *unit)
        : mKey(key), mLabel(label), mUnit(unit ? unit : "") {}
    virtual ~Field() = default;

    const char *key() const { return mKey; }
    const char *label() const { return mLabel; }
    const char *unit() const { return mUnit; }

    // Heading of the group the field starts, nullptr if none
    const char *heading() const { return mSection; }

    // Shown on the dashboard of the given access level
    bool shown(Access access) const;
    bool editable() const { return mEditable; }

    // Current value as shown on the pages
    virtual std::string text() const = 0;

    // Parse a form value, false if it is invalid
    virtual bool stage(const std::string &text) = 0;
    // The staged value differs from the current one
    virtual bool pending() const = 0;
    // Store the staged value
    virtual void commit() = 0;
    virtual void discard() = 0;

    // Form input attributes
    virtual size_t maxLength() const = 0;
    virtual const char *inputMode() const = 0;

protected:
    enum class Show : uint8_t {
        Hidden,
        Local,
        Public,
    };

    const char *mKey;
    const char *mLabel;
    const char *mUnit;
    const char *mSection = nullptr;
    Show mShow = Show::Local;
    bool mEditable = false;
};

using Fields = std::vector<std::unique_ptr<Field>>;

template <typename T>
class ValueField : public Field {
public:
    using Getter = std::function<T()>;
    using Setter = std::function<void(const T &)>;

    ValueField(const char *key, const char *label, const char *unit, Getter get, Setter store)
        : Field(key, label, unit), mGet(std::move(get)), mStore(std::move(store)) {}

    // Show on the public dashboard too
    ValueField &publish() { mShow = Show::Public; return *this; }
    // Don't show on the dashboard, e.g. a setting
    ValueField &hide() { mShow = Show::Hidden; return *this; }
    // Start a group with a heading on the dashboard and the settings page
    ValueField &section(const char *title) { mSection = title; return *this; }

    // Allow to change on the settings page; onChange is called after the
    // value is stored, to apply it (e.g. to a sensor)
    ValueField &editable(Setter onChange = nullptr)
    {
        mEditable = true;
        mOnChange = std::move(onChange);
        return *this;
    }

    T value() const { return mGet(); }

    bool pending() const override
    {
        // The unchanged text of a rounded value (a float shown with fewer
        // decimals) must not overwrite the value
        return mStaged.has_value() && (*mStaged != mGet()) && (mStagedText != text());
    }

    void commit() override
    {
        if (!mStaged)
            return;

        T value = std::move(*mStaged);
        discard();

        if (mStore)
            mStore(value);
        if (mOnChange)
            mOnChange(value);
    }

    void discard() override
    {
        mStaged.reset();
        mStagedText.clear();
    }

protected:
    void staged(T value, const std::string &text)
    {
        mStaged = std::move(value);
        mStagedText = text;
    }

    Getter mGet;
    Setter mStore;
    Setter mOnChange;
    std::optional<T> mStaged;
    std::string mStagedText;
};

// Unsigned counter stored in small units and shown in scale units, e.g.
// liters shown as m3 with scale 1000
class CounterField : public ValueField<uint32_t> {
public:
    CounterField(const char *key, const char *label, const char *unit,
                 uint32_t scale, uint8_t decimals, Getter get, Setter store);

    std::string text() const override;
    bool stage(const std::string &text) override;
    size_t maxLength() const override;
    const char *inputMode() const override { return "decimal"; }

private:
    uint32_t mScale;
    uint8_t mDecimals;
};

class FloatField : public ValueField<float> {
public:
    FloatField(const char *key, const char *label, const char *unit,
               uint8_t decimals, Getter get, Setter store);

    std::string text() const override;
    bool stage(const std::string &text) override;
    size_t maxLength() const override { return 16; }
    // Not "decimal": phone keyboards may have no minus there
    const char *inputMode() const override { return "text"; }

private:
    uint8_t mDecimals;
};

class TextField : public ValueField<std::string> {
public:
    TextField(const char *key, const char *label, size_t maxLength, Getter get, Setter store);

    std::string text() const override { return mGet(); }
    bool stage(const std::string &text) override;
    size_t maxLength() const override { return mMaxLength; }
    const char *inputMode() const override { return "text"; }

private:
    size_t mMaxLength;
};

} // namespace portal
