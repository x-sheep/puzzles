using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace PuzzleModern.UWP
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HelpFlyout
    {
        private readonly UISettings uiSettings;
        private bool isNavigation = true;

        static readonly Color DarkBackground = Color.FromArgb(0xFF, 0x2B, 0x2B, 0x2B);
        static readonly Color LightBackground = Color.FromArgb(0xFF, 0xEE, 0xEE, 0xEE);

        public HelpFlyout(string page = "index")
        {
            this.InitializeComponent();

            var url = "ms-appx-web:///" + page + ".html";
            HelpView.Navigate(new Uri(url));
            Width = Window.Current.Bounds.Width < 646 ? 346 : 646;

            uiSettings = new Windows.UI.ViewManagement.UISettings();
            uiSettings.ColorValuesChanged += (sender, args) => { ApplyColors(); };
            ApplyColors();
        }

        private void ApplyColors()
        {
            _ = Dispatcher.RunAsync(CoreDispatcherPriority.Normal, async () =>
            {
                var isDark = uiSettings.GetColorValue(UIColorType.Background) == Colors.Black;
                HelpView.DefaultBackgroundColor = isDark ? DarkBackground : LightBackground;
                if (isNavigation) return;

                if (isDark)
                    await HelpView.InvokeScriptAsync("eval", new[] { "document.body.className = \"dark\"" });
                else
                    await HelpView.InvokeScriptAsync("eval", new[] { "document.body.className = \"light\"" });
            });
        }

        private void HelpFlyout_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            HelpView.Width = e.NewSize.Width;
            HelpView.Height = e.NewSize.Height - 80;
        }

        private void HelpView_NavigationStarting(WebView sender, WebViewNavigationStartingEventArgs args)
        {
            isNavigation = true;
            if (args.Uri.Scheme == "http" || args.Uri.Scheme == "https")
            {
                args.Cancel = true;
                _ = Windows.System.Launcher.LaunchUriAsync(args.Uri);
            }
        }

        private void HelpView_NavigationCompleted(WebView sender, WebViewNavigationCompletedEventArgs args)
        {
            isNavigation = false;
            ApplyColors();
        }

        private void HelpFlyout_Loaded(object sender, RoutedEventArgs e)
        {
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated += OnAcceleratorKeyActivated;
        }

        private void HelpFlyout_Unloaded(object sender, RoutedEventArgs e)
        {
            Window.Current.CoreWindow.Dispatcher.AcceleratorKeyActivated -= OnAcceleratorKeyActivated;
        }

        private void OnAcceleratorKeyActivated(CoreDispatcher sender, AcceleratorKeyEventArgs args)
        {
            if (args.VirtualKey == Windows.System.VirtualKey.Escape)
            {
                Hide();
                args.Handled = true;
            }
        }
    }
}
