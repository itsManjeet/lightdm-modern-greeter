#include <gtk/gtk.h>
#include <lightdm.h>

#include <releax/config.hh>

#define CFG_FILE "/etc/greeter.conf"

LightDMGreeter *greeter;
GtkEntry  *UserEntry,
          *PasswordEntry;
GtkWidget *MainWindow;
GtkLabel  *MessageLabel;

Config* cfg;
const char*   session;


void
login_func()
{
    gtk_label_set_text(MessageLabel, "");

    if (lightdm_greeter_get_is_authenticated(greeter)) {
        lightdm_greeter_start_session_sync(greeter, session, NULL);
    } else if (lightdm_greeter_get_in_authentication(greeter)) {
        lightdm_greeter_respond(greeter, gtk_entry_get_text(PasswordEntry),NULL);
    } else {
        lightdm_greeter_authenticate(greeter, gtk_entry_get_text(UserEntry), NULL);
    }
}

void
show_message_func(LightDMGreeter   *ldm,
                  const gchar      *text,
                  LightDMPromptType type)
{
    gtk_label_set_text(MessageLabel, text);
}

void
show_prompt_func(LightDMGreeter   *ldm,
                 const gchar      *text,
                 LightDMPromptType type)
{

}

void
auth_pass_func(LightDMGreeter *ldm)
{
    if (!lightdm_greeter_get_is_authenticated(ldm)) {
        gtk_label_set_text(MessageLabel,"Authentication failure");
    } else if (!lightdm_greeter_start_session_sync(ldm,session,NULL)) {
        gtk_label_set_text(MessageLabel, "failed to start session");
    }

    lightdm_greeter_authenticate(ldm,NULL,NULL);
}

int 
main(int    ac,  
     char** av)
{

    GtkCssProvider  *css_provider;
    GtkBuilder      *builder;
    GdkScreen       *screen;
    GdkDisplay      *display;
    GdkMonitor      *monitor;
    GdkRectangle    geometry;


    gtk_init(&ac, &av);

    cfg  = new Config(CFG_FILE);

    session      = cfg->get("session","default").c_str();
    css_provider = gtk_css_provider_new();
    display      = gdk_display_get_default();
    screen       = gdk_display_get_default_screen(display);

    builder = gtk_builder_new();


    gtk_builder_add_from_file(builder,"/usr/share/lightdm/modern.ui",NULL);

    MainWindow     = GTK_WIDGET(gtk_builder_get_object(builder,"MainWindow"));
    UserEntry      = GTK_ENTRY (gtk_builder_get_object(builder,"UserEntry"));
    PasswordEntry  = GTK_ENTRY (gtk_builder_get_object(builder,"PasswordEntry"));
    MessageLabel   = GTK_LABEL (gtk_builder_get_object(builder,"MessageLabel"));
    GtkButton *LoginButton   = GTK_BUTTON (gtk_builder_get_object(builder,"LoginButton"));

    // connect gtk signals
    g_signal_connect (PasswordEntry, "activate", G_CALLBACK (login_func), NULL);
    g_signal_connect (LoginButton,   "activate", G_CALLBACK (login_func), NULL);

    // show window
    monitor = gdk_display_get_primary_monitor (display);
    gdk_monitor_get_geometry (monitor, &geometry);
    gtk_window_set_default_size (GTK_WINDOW (MainWindow), geometry.width, geometry.height);

    gtk_widget_show (MainWindow);
    gtk_entry_grab_focus_without_selecting (UserEntry);


    greeter = lightdm_greeter_new ();

    // connect lightdm signals
    g_signal_connect (greeter, "show-prompt", G_CALLBACK (show_prompt_func), NULL);
    g_signal_connect (greeter, "show-message", G_CALLBACK (show_message_func), NULL);
    g_signal_connect (greeter, "authentication-complete", G_CALLBACK (auth_pass_func), NULL);

    // connect to lightdm daemon
    if (!lightdm_greeter_connect_to_daemon_sync (greeter, NULL))
        return EXIT_FAILURE;

    // start authentication
    lightdm_greeter_authenticate (greeter, NULL, NULL);


    gtk_main();

    return EXIT_SUCCESS;
}