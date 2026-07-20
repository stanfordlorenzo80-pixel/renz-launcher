const fs = require('fs');
const p = 'C:/Users/Administrator/Desktop/renz_launcher/renz_launcher.py';
let c = fs.readFileSync(p, 'utf8');

c = c.replace(/def _build_ui\(self\):/, `def _launch_worm_proxy(self):
        import subprocess
        import os
        disable_think = "1" if getattr(self, "v_disable_think", ctk.BooleanVar(value=False)).get() else "0"
        env = os.environ.copy()
        env["DISABLE_THINKING"] = disable_think
        script_path = os.path.join(os.path.dirname(__file__), "proxy_server.py")
        subprocess.Popen([sys.executable, script_path], env=env, creationflags=subprocess.CREATE_NEW_CONSOLE)
        self._set_status(f"WORM Proxy started on port 11435. (Thinking disabled: {disable_think == '1'})", ok=True)

    def _build_ui(self):
        self.v_disable_think = ctk.BooleanVar(value=False)`);

c = c.replace(/command=self\._load_nova_into_textbox\s*\)\.pack\(side="right", padx=\(8, 0\)\)/, `command=self._load_nova_into_textbox
            ).pack(side="right", padx=(8, 0))

            ctk.CTkButton(
                header_row, text="Launch WORM Proxy",
                width=120, height=26,
                font=("Segoe UI", 10, "bold"), corner_radius=4,
                fg_color="#b30000", hover_color="#cc0000",
                text_color="#ffffff",
                command=self._launch_worm_proxy
            ).pack(side="right", padx=(8, 0))

            ctk.CTkCheckBox(
                header_row, text="Disable <think>",
                variable=self.v_disable_think,
                font=("Segoe UI", 10),
                width=100
            ).pack(side="right", padx=(8, 0))`);

fs.writeFileSync(p, c);
