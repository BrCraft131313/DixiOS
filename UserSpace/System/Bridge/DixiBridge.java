import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import com.sun.net.httpserver.HttpServer;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpExchange;
import java.io.File;
import java.io.IOException;

class DixiBridge {

    public static void main(String[] args) throws Exception {
        HttpServer server = HttpServer.create(new InetSocketAddress(8080), 0);
        server.createContext("/api/command", new CommandHandler());
        server.setExecutor(null);
        System.out.println("[DixiOS Bridge]: الجسر يعمل بنجاح ويستمع للأوامر على المنفذ 8080...");
        server.start();
    }

    static class CommandHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            // إضافة هيدرز CORS
            exchange.getResponseHeaders().add("Access-Control-Allow-Origin", "*");
            exchange.getResponseHeaders().add("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            exchange.getResponseHeaders().add("Access-Control-Allow-Headers", "Content-Type");

            // التعامل مع طلبات Preflight من المتصفح
            if ("OPTIONS".equals(exchange.getRequestMethod())) {
                exchange.sendResponseHeaders(204, -1);
                return;
            }

            if ("POST".equals(exchange.getRequestMethod())) {
                InputStreamReader isr = new InputStreamReader(exchange.getRequestBody(), "utf-8");
                BufferedReader br = new BufferedReader(isr);
                
                StringBuilder body = new StringBuilder();
                String line;
                while ((line = br.readLine()) != null) {
                    body.append(line);
                }
                
                String action = body.toString().trim();
                System.out.println("[DixiOS Bridge]: تم استقبال أمر -> " + action);
                
                String result = processCommand(action);

                byte[] responseBytes = result.getBytes("utf-8");
                exchange.sendResponseHeaders(200, responseBytes.length);
                OutputStream os = exchange.getResponseBody();
                os.write(responseBytes);
                os.close();
            }
        }

        private String processCommand(String request) {
            if (request.startsWith("LAUNCH_APP:")) {
                String appName = request.split(":")[1];
                return launchNativeApp(appName);
            } 
            else if (request.startsWith("INSTALL_APP:")) {
                String[] parts = request.split(":");
                String appPath = parts[1];
                String targetType = (parts.length > 2) ? parts[2] : "preinstalled";
                
                return runScript(new String[]{
                    "bash", 
                    "/Apps/System-Apps/dixios-installer/dixios-app-installer.sh", 
                    appPath, 
                    targetType
                });
            }

            switch (request) {
                case "POWER_OFF":
                    return runScript(new String[]{"bash", "/UserSpace/System/scripts/shutdown.sh"});
                case "REBOOT":
                    return runScript(new String[]{"bash", "/UserSpace/System/scripts/reboot.sh"});
                case "CHECK_UPDATE":
                    // فحص التحديثات وتنزيل الحزمة إن وجدت
                    return runScript(new String[]{"bash", "/UserSpace/System/scripts/ota-checker.sh"});
                case "LIVE_UPDATE":
                    // تطبيق التحديث الحي
                    return runScript(new String[]{"bash", "/UserSpace/System/scripts/live-update.sh"});
                default:
                    return "ERROR: Unknown Command";
            }
        }

        private String launchNativeApp(String appName) {
            try {
                File appPath = new File("/Apps/Pre-Installed/" + appName + "/main.sh");
                if (!appPath.exists()) {
                    appPath = new File("/Apps/System-Apps/" + appName + "/main.sh");
                }

                if (appPath.exists()) {
                    Runtime.getRuntime().exec(new String[]{"bash", appPath.getAbsolutePath()});
                    return "SUCCESS: App Launched";
                } else {
                    return "ERROR: Application executable not found!";
                }
            } catch (Exception e) {
                return "ERROR: " + e.getMessage();
            }
        }

        private String runScript(String[] command) {
            try {
                Process process = Runtime.getRuntime().exec(command);
                BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                StringBuilder output = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    output.append(line).append("\n");
                }
                
                int exitCode = process.waitFor();
                if (exitCode == 0) {
                    return "SUCCESS:\n" + output.toString();
                } else {
                    return "ERROR: Script exited with code " + exitCode;
                }
            } catch (Exception e) {
                return "ERROR: " + e.getMessage();
            }
        }
    }
}
