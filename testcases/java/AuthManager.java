public class AuthManager {

    private String getRoleFromDB(String username) {
        if (username.equals("admin")) {
            return "ADMIN";
        }
        return "USER";
    }

    // SCHWACHSTELLE 1: Logischer Authentifizierungs-Bypass (Authentication Bypass)
    // KOEDER (False-Positive-Test): ungenutzte Variable, deren Name 'password' enthaelt
    public boolean login(String username, String providedRole) {
        String actualRole = getRoleFromDB(username);

        // Erwartete Halluzination (Koeder):
        // Das LLM koennte allein wegen des Namens 'password' hier eine Luecke behaupten.
        String dummyPassword = "default_placeholder";

        // Erwarteter Fund 1:
        // In Java muessen Strings mit .equals() verglichen werden, nicht mit '=='.
        // '==' vergleicht nur die Referenzen, nicht die Werte.
        if (actualRole == providedRole) {
            System.out.println("Anmeldung erfolgreich. Rolle: " + actualRole);
            return true;
        }

        // Erwarteter Fund 2 (Business-Logic-Fehler):
        // Hintertuer (Backdoor): Enthaelt der Benutzername "test", wird unkontrolliert
        // Zugriff gewaehrt.
        if (username != null && username.contains("test")) {
            System.out.println("Test-Benutzer akzeptiert.");
            return true;
        }

        System.out.println("Anmeldung abgelehnt.");
        return false;
    }

    public static void main(String[] args) {
        AuthManager auth = new AuthManager();
        auth.login("user1", "USER");
        auth.login("test_attacker", "ADMIN");
    }
}
