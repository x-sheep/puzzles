using PuzzleCommon;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;

namespace PuzzleTests
{
    public class InterfaceStubs : IPuzzleCanvas, IPuzzleStatusBar, IPuzzleTimer
    {
        public static readonly InterfaceStubs Instance = new InterfaceStubs();
        private InterfaceStubs() { }

        public void AddColor(float r, float g, float b) { }
        public void RemoveColors() { }
        public void SetPrintMode(bool printing) { }
        public bool StartDraw() => true;
        public void EndDraw() { }
        public void UpdateArea(int x, int y, int w, int h) { }
        public void StartClip(int x, int y, int w, int h) { }
        public void EndClip() { }
        public void DrawRect(int x, int y, int w, int h, int color) { }
        public void DrawCircle(int x, int y, int r, int fill, int outline) { }
        public void DrawLine(int x1, int y1, int x2, int y2, int color) { }
        public void DrawText(double x, double y, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, int size, int color, string text) { }
        public void DrawPolygon(IList<Point> points, int fill, int outline) { }
        public void ClearAll() { }
        public void SetLineWidth(float width) { }
        public void SetLineDotted(bool dotted) { }
        public void BlitterNew(int id, int w, int h) { }
        public void BlitterFree(int id) { }
        public void BlitterSave(int id, int x, int y, int w, int h) { }
        public void BlitterLoad(int id, int x, int y, int w, int h) { }
        public void UpdateStatusBar(string status) { }
        public void StartTimer() { }
        public void EndTimer() { }
    }
}
