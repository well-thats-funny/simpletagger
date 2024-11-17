/*
    Copyright (C) 2024 fdresufdresu@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

struct MemoryState {
    std::int64_t total;
    std::int64_t resident;
    std::int64_t shared;
    std::int64_t text;
    std::int64_t data;
};

std::optional<MemoryState> fetchMemoryUsage();

std::optional<int> backupFile(QString const &fileName);

QSize toMinimalAspectRatio(QSize const &aspectRatio);

// point on a line specified as ax+by+c=0 that is closest to p
QPointF closestPointOnLine(qreal a, qreal b, qreal c, QPointF const &p);

// point on a line specified as y=ax+b that is closest to p
QPointF closestPointOnLine(qreal a, qreal b, QPointF const &p);

QString cborTypeToString(QCborValue::Type type);

/**
 * dynamic_cast operation for std::unique_ptr
 *
 * This allows for dynamic_cast to be performed on an unique_ptr. If it succeeds, the returned pointer becomes the
 * owner of the stored value and the "ptr" argument becomes nullptr. If it fails, the returned pointer is nullptr and
 * the "ptr" argument retains ownership of the value.
 *
 * This is only for cases where you want the ownership to be transferred to the target pointer. If you need a cast
 * that doesn't transfer ownership, just use @c dynamic_cast<Target*>(ptr.get()) instead.
 */
template<typename TargetType, typename TargetDeleter = std::default_delete<TargetType>>
std::unique_ptr<TargetType, TargetDeleter> dynamicPtrCast(auto &&ptr) {
    if (auto targetRaw = dynamic_cast<TargetType *>(ptr.get()); !targetRaw)
        // NOTE: if dynamic_cast fails, returns a null pointer (like for builtin pointers), but does not move out
        //       from "ptr" - so you can continue to use it after a failed cast
        return nullptr;
    else {
        ptr.release();
        return std::unique_ptr<TargetType, TargetDeleter>{targetRaw};
    }
}

template<typename R>
std::generator<std::ranges::range_value_t<R>> withoutDuplicates(R &&r) {
    std::set<std::ranges::iterator_t<R>> seen;

    for (auto it = std::begin(r); it != std::end(r); ++it) {
        if (std::ranges::none_of(seen, [&](auto const &v){ return *v == *it; })) {
            seen.insert(it);
            co_yield *it;
        }
    }
}

namespace impl {
template<typename A, typename B>
struct copyConst {
    using tA = std::remove_reference_t<A>;
    using tB = std::remove_cvref_t<B>;
    using type = std::conditional_t<std::is_const_v<tA>, std::add_const_t<tB>, tB>;
};
}

// copy constness category from type A and apply it to type B
template<typename A, typename B>
using copyConst = impl::copyConst<A, B>::type;

using Error = QString;
struct CancelOperation {};

using ErrorOrCancel = std::variant<Error, CancelOperation>;

void reportError(QString const &title, Error const &content, bool messageBox = true);
