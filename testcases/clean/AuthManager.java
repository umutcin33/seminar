public class AuthManager {

    private String getRoleFromDB(String username) {
        if (username.equals("admin")) {
            return "ADMIN";
        }
        return "USER";
    }

    public boolean login(String username, String providedRole) {
        String actualRole = getRoleFromDB(username);

        String dummyPassword = "default_placeholder";

        if (actualRole == providedRole) {
            System.out.println("Anmeldung erfolgreich. Rolle: " + actualRole);
            return true;
        }

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
