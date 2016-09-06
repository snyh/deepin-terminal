[DBus (name = "com.deepin.terminal.PtyFilter")]
public class PtyFilterProxy : Object {
	public int register_output_filter(uint8[] tag) {
		return 0;
	}
	
	public int register_input_filter(uint8[] tag) {
		return 0;
	}

	public void spwan_at_pty(string workdir, string[] argv, string[]argc) {

	}

	public bool found { get; set; }
	
	public signal void pty_special_output(uint8[] output);
}

void on_bus_aquired (DBusConnection conn, string name, PtyFilterProxy s) {
    try {
        conn.register_object ("/", s);
    } catch (IOError e) {
        stderr.printf ("Could not register service\n");
    }
}

void dbus_init(PtyFilterProxy proxy, string addr) {
	Bus.own_name (BusType.SESSION, addr, BusNameOwnerFlags.NONE,
                  (c, n) => { on_bus_aquired(c, n, proxy); },
                  () => {},
                  () => stderr.printf ("Could not aquire name\n"));
}