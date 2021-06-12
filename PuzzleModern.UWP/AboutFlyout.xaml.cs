using Windows.UI.Xaml;

namespace PuzzleModern.UWP
{
    public sealed partial class AboutFlyout
    {
        public AboutFlyout()
        {
            this.InitializeComponent();
            Width = Window.Current.Bounds.Width < 646 ? 346 : 646;
        }
    }
}
