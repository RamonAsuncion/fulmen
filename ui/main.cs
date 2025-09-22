using System;
using System.Runtime.InteropServices;

class ContainerUI
{
  private const string FULMEN_CONTAINER_BACKEND_LIB = "libfulmen_backend.so";

  [DllImport(FULMEN_CONTAINER_BACKEND_LIB, CallingConvention = CallingConvention.Cdecl)]
  public static extern int container_start(string name);

  [DllImport(FULMEN_CONTAINER_BACKEND_LIB, CallingConvention = CallingConvention.Cdecl)]
  public static extern int container_stop(string name);

  static void Main()
  {
    string container_name = "Container 1";
    Console.WriteLine("[ui] Starting container...");
    container_start(container_name);
    Console.WriteLine("[ui] Stopping container...");
    container_stop(container_name);
  }
}
