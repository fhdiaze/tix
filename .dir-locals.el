;;; Directory-local settings for tix (same as .vscode/launch.json + tasks.json)
;; Debugger: CodeLLDB from VS Code ("codelldb-vscode", set up in init.el).
((nil . ((compile-command . "misc\\build.bat /m debug")
         (dape-command . (codelldb-vscode
                          compile "misc\\build.bat /m debug"
                          :program "C:/Users/Fredy.Diaz/Documents/Main/Personal/Code/tix/bin/tix_win.exe"
                          :cwd "C:/Users/Fredy.Diaz/Documents/Main/Personal/Code/tix/data")))))
