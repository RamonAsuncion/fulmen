using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Layout;
using System.Runtime.InteropServices;
using Avalonia.Platform.Storage;

namespace FulmenUI
{
    public class App : Application
    {
        public override void OnFrameworkInitializationCompleted()
        {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                desktop.MainWindow = new MainWindow();
            }
            base.OnFrameworkInitializationCompleted();
        }
    }

    public class MainWindow : Window
    {
#if __OSX__
        [DllImport("libfulmen_backend.dylib", CallingConvention = CallingConvention.Cdecl)]
#else
        [DllImport("libfulmen_backend.so", CallingConvention = CallingConvention.Cdecl)]
#endif
        public static extern int run_container(string image_path, string command);

        public MainWindow()
        {
            Title = "Fulmen Container Launcher";
            Width = 400;
            Height = 200;

            var imagePathBox = new TextBox
            {
                Width = 300,
                Watermark = "Image Tarball Path"
            };

            var browseButton = new Button { Content = "Browse" };

            var commandBox = new TextBox
            {
                Width = 300,
                Text = "/bin/echo Hello",
                Watermark = "Command"
            };

            var runButton = new Button { Content = "Run Container" };

            var statusLabel = new TextBlock { Width = 350 };

            var panel = new StackPanel { Margin = new Thickness(10) };

            panel.Children.Add(new TextBlock { Text = "Image Tarball:" });

            var row1 = new StackPanel { Orientation = Orientation.Horizontal };
            row1.Children.Add(imagePathBox);
            row1.Children.Add(browseButton);
            panel.Children.Add(row1);

            panel.Children.Add(new TextBlock { Text = "Command:" });
            panel.Children.Add(commandBox);

            panel.Children.Add(runButton);
            panel.Children.Add(statusLabel);

            Content = panel;

            browseButton.Click += async (_, __) =>
            {
                var picker = this.StorageProvider;

                var options = new FilePickerOpenOptions
                {
                    Title = "Select Image Tarball",
                    AllowMultiple = false
                };

                var result = await picker.OpenFilePickerAsync(options);
                if (result != null && result.Count > 0)
                {
                    var file = result[0];
                    var path = await file.OpenReadAsync();
                    imagePathBox.Text = file.Name ?? "";
                }
            };

            runButton.Click += (_, __) =>
            {
                statusLabel.Text = "Running...";
                try
                {
                    int code = run_container(imagePathBox.Text, commandBox.Text);
                    statusLabel.Text = $"Container exited with code {code}";
                }
                catch (System.Exception ex)
                {
                    statusLabel.Text = $"Error: {ex.Message}";
                }
            };
        }
    }

    class Program
    {
        public static void Main(string[] args)
        {
            BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        }

        public static AppBuilder BuildAvaloniaApp()
            => AppBuilder.Configure<App>()
                .UsePlatformDetect()
                .LogToTrace();
    }
}
