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
#include "TagProcessor.hpp"

namespace TagProcessor {
namespace {
QString resolveParametrized(QString const &string, QString const &spec) {
    QString result;

    auto ch = string.begin();

    enum class State { OUTSIDE, IN_NAME, IN_REPL };
    State state = State::OUTSIDE;
    QString currentName;
    QString currentRepl;

    while (ch != string.end()) {
        switch (state) {
            case State::OUTSIDE: {
                if (ch->unicode() == '(')
                    state = State::IN_NAME;
                else
                    result += *ch;
                ++ch;
                break;
            }
            case State::IN_NAME: {
                if (ch->unicode() == ':')
                    state = State::IN_REPL;
                else
                    currentName += *ch;
                ++ch;
                break;
            }
            case State::IN_REPL: {
                if (ch->unicode() == ')') {
                    // include this part only if it's specified as spec
                    if (currentName == spec)
                        result += currentRepl;

                    currentName.clear();
                    currentRepl.clear();
                    state = State::OUTSIDE;
                } else {
                    currentRepl += *ch;
                }
                ++ch;
                break;
            }
        }
    }

    return result;
}
}

QString resolveChildTag(QString const &parent, QString const &child) {
    ZoneScoped;

    QString result;
    // TODO: result.reserve() ?

    enum class State { OUTSIDE, IN_PARAMS };
    State state = State::OUTSIDE;

    QString currentSpec;

    auto ch = child.begin();
    while (ch != child.end()) {
        switch (state) {
            case State::OUTSIDE: {
                if (ch->unicode() == '%') {
                    ++ch;

                    if (ch == child.end()) {
                        result += resolveParametrized(parent, "");
                        break;
                    }

                    if (ch->unicode() == '(') {
                        state = State::IN_PARAMS;
                        ++ch;
                    } else {
                        result += resolveParametrized(parent, "");
                    }
                } else {
                    result += *ch;
                    ++ch;
                }
                break;
            }
            case State::IN_PARAMS: {
                if (ch->unicode() == ')') {
                    result += resolveParametrized(parent, currentSpec);
                    currentSpec.clear();
                    state = State::OUTSIDE;
                } else {
                    currentSpec += *ch;
                }
                ++ch;
                break;
            }
        }
    }

    return resolveParametrized(result, "");
}
}
