import QtQuick 2.0

// ===================================================================
// Helper to handle button click outside a component.
// ===================================================================

Item {
    id: item

    property var _mouseArea

    signal pressed

    function _createMouseArea () {
        if (_mouseArea == null) {
            _mouseArea = builder.createObject(this)
        }

        _mouseArea.parent = (function () {
            // Search root.
            var p = item

            while (p.parent != null) {
                p = p.parent
            }

            return p
        })()
    }

    function _deleteMouseArea () {
        if (_mouseArea != null) {
            _mouseArea.destroy()
            _mouseArea = null
        }
    }

    function _isInItem (point) {
        return (
            point.x >= item.x &&
            point.y >= item.y &&
            point.x <= item.x + item.width &&
            point.y <= item.y + item.height
        )
    }

    // It's necessary to use a `enabled` variable.
    // See: http://doc.qt.io/qt-5/qml-qtqml-component.html#completed-signal
    //
    // The creation order of components in a view is undefined,
    // so the mouse area mustt be created only when `enabled == true`.
    //
    // In the first view render, `enabled` must equal false.
    Component.onCompleted: enabled && _createMouseArea()
    Component.onDestruction: _deleteMouseArea()

    onEnabledChanged: {
        _deleteMouseArea()

        if (enabled) {
            _createMouseArea()
        }
    }

    Component {
        id: builder

        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            z: 9999999999 // Ugly! But it's necessary in some cases...

            onPressed: {
                // Propagate event.
                mouse.accepted = false

                if (!_isInItem(
                    mapToItem(item.parent, mouse.x, mouse.y)
                )) {
                    // Outside!!!
                    item.pressed()
                }
            }
        }
    }
}
