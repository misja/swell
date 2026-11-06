// SPDX-License-Identifier: MIT
// Negative control: a property that does not exist must fail to compile.
import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    noSuchProperty: 1
}
