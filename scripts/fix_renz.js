const fs = require('fs');
const p = 'C:/Users/Administrator/Desktop/renz_launcher/renz_launcher.py';
let c = fs.readFileSync(p, 'utf8');

const search = `        def _launch_worm_proxy(self):
        import subprocess
        import os
        disable_think = "1" if getattr(self, "v_disable_think", ctk.BooleanVar(value=False)).get() else "0"
        env = os.environ.copy()
        env["DISABLE_THINKING"] = disable_think
        script_path = os.path.join(os.path.dirname(__file__), "proxy_server.py")
        subprocess.Popen([sys.executable, script_path], env=env, creationflags=subprocess.CREATE_NEW_CONSOLE)
        self._set_status(f"WORM Proxy started on port 11435. (Thinking disabled: {disable_think == '1'})", ok=True)

    def _build_ui(self):
        self.v_disable_think = ctk.BooleanVar(value=False)`;

const replace = `        def _launch_worm_proxy(self):
            import subprocess
            import os
            import sys
            disable_think = "1" if getattr(self, "v_disable_think", ctk.BooleanVar(value=False)).get() else "0"
            env = os.environ.copy()
            env["DISABLE_THINKING"] = disable_think
            script_path = os.path.join(os.path.dirname(__file__), "proxy_server.py")
            subprocess.Popen([sys.executable, script_path], env=env, creationflags=subprocess.CREATE_NEW_CONSOLE)
            self._set_status(f"WORM Proxy started on port 11435. (Thinking disabled: {disable_think == '1'})", ok=True)

        def _build_ui(self):
            self.v_disable_think = ctk.BooleanVar(value=False)`;

c = c.replace(search, replace);
fs.writeFileSync(p, c);
