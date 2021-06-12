using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
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
    public sealed partial class SpecificDialog
    {
        public SpecificDialog(string gameID, string title)
        {
            this.InitializeComponent();

            Title = title;
            PromptPopupText.Text = gameID;
        }

        public event EventHandler<NewGameIDEventArgs> NewGameID;

        private void ContentDialog_PrimaryButtonClick(ContentDialog sender, ContentDialogButtonClickEventArgs args)
        {
            var newArgs = new NewGameIDEventArgs { NewID = PromptPopupText.Text };

            NewGameID?.Invoke(this, newArgs);

            if (newArgs.Error != null)
            {
                args.Cancel = true;
                PromptPopupErrorLabel.Text = newArgs.Error;
                ErrorAppearingStoryboard.Begin();
            }
        }
    }

    public class NewGameIDEventArgs
    {
        private string _error;
        public string NewID { get; set; }
        public string Error
        {
            get => _error;
            set
            {
                if (value != null)
                    _error = value;
            }
        }
    }
}
