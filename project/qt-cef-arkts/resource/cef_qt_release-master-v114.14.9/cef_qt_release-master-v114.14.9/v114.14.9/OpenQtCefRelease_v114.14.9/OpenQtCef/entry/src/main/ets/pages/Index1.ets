import uri from '@ohos.uri';
import { JsPasteBoard } from '../native/QtCore/JsPasteBoard'
import pasteboard from '@ohos.pasteboard';
@Entry
@Component
struct Index1 {
  build() {
    Column() {
      Button("test").width("100%").onClick((event)=>{
          // let pasteBoard = new JsPasteBoard;
          // pasteBoard.clipboardText().then((data: string) => {
          //   console.log("hhhhhhhhhhhhhhhhhhh", data);
          // })
        let systemPasteboard = pasteboard.getSystemPasteboard();
        let pasteData = systemPasteboard.getData();
        pasteData.then(p=>{
          console.log("yyyyyyyyyyyyyyyyyyyyyyyyyyyy", p.getPrimaryText())
        });
      })
    }
  }
}
