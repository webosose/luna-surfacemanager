import QtQuick 2.4

Item {
    id: scrollPage
    visible: true

    Rectangle {
        id: page
        color: "yellow"
        width: animationArea.width * 2 / 3
        height: animationArea.height
        border.color: "black"
        border.width: 2
        clip: true

        Flickable {
            id: flickable
            anchors.fill: parent
            contentWidth: textPage.width
            contentHeight: textPage.height
            flickableDirection: Flickable.VerticalFlick

            Column {
                id: textPage

                Text {
                    font.pixelSize: 50
                    font.weight: Font.Normal
                    horizontalAlignment: Text.AlignJustify
                    text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit." + "\n" + "Sed posuere mauris non malesuada semper." + "\n" + "Sed pulvinar velit condimentum metus mollis, ac finibus metus molestie." + "\n" + "Nullam sollicitudin neque vel metus vestibulum dapibus." + "\n" + "Mauris sit amet risus ullamcorper tellus malesuada iaculis a rhoncus nisl." + "\n" + "Fusce laoreet blandit faucibus." + "\n" + "Maecenas a sem eget nibh pellentesque dignissim." + "\n" + "Duis condimentum arcu sed vestibulum interdum." + "\n" + "Nulla facilisi." + "\n" + "Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas." + "\n\n" +
"Suspendisse aliquet pretium bibendum." + "\n" + "Sed tincidunt diam dui, vitae congue magna pharetra ut." + "\n" + "Suspendisse et fringilla metus." + "\n" + "Donec laoreet sit amet turpis vitae dapibus." + "\n" + "Etiam auctor vestibulum augue, non pellentesque lacus condimentum in." + "\n" + "Donec dictum odio sit amet bibendum suscipit." + "\n" + "Nam feugiat lacus vitae tincidunt tempor." + "\n" + "Donec lacus felis, bibendum ut neque sit amet, imperdiet sagittis urna." + "\n" + "Ut feugiat justo quam, lobortis consequat magna molestie quis." + "\n" + "Praesent ac purus nec ante ultrices rutrum." + "\n" + "In quis tellus ullamcorper, vehicula nulla in, pellentesque libero." + "\n" + "Pellentesque consequat lacus vel tortor fringilla fermentum." + "\n" + "Mauris blandit arcu risus, sit amet dictum nisi tempus eu." + "\n" + "Cras eleifend rhoncus sem, eget vestibulum arcu venenatis nec." + "\n" + "Etiam lacus dolor, egestas vel lectus vel, consectetur ultricies mauris." + "\n" + "Pellentesque quis suscipit sem." + "\n\n" +
"Nam congue luctus pellentesque." + "\n" + "Nullam pretium lacinia malesuada." + "\n" + "Pellentesque vel odio a urna egestas posuere." + "\n" + "Cras sit amet felis libero." + "\n" + "Quisque feugiat posuere lectus at maximus." + "\n" + "Mauris id urna quis ex aliquam consequat." + "\n" + "Nulla hendrerit vulputate dolor placerat eleifend." + "\n" + "Duis vestibulum pulvinar purus, sed lacinia nisl luctus sit amet." + "\n" + "Curabitur condimentum turpis quam, a rhoncus ex porttitor vitae." + "\n" + "Nullam mauris mauris, venenatis id dictum et, volutpat a felis." + "\n" + "Donec vel velit erat." + "\n" + "Ut at porta augue, vel scelerisque risus." + "\n" + "Nunc felis magna, posuere vel dapibus vel, dictum ut augue." + "\n" + "Sed dapibus risus vitae enim ultrices, at cursus nibh volutpat."
                }
            }
        }
    }
}
