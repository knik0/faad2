/*
** Dialog info file 
** use gtk 1.2.x
** with the help of wGlade
*/

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

void updateWindowDatas(void);


GtkWidget *createDialogInfo(void);
char *title=0, *artist=0, *album=0, *year=0, *track=0, *genre=0, *comment=0,
	*composer=0, *url=0, *originalArtist=0, *encodedby=0;

GtkWidget* createDialogInfo(void)
{
  GtkWidget *window1;
  GtkWidget *fixed1;
  GtkWidget *button2;
  GtkWidget *frame2;
  GtkWidget *fixed3;
  GtkWidget *Track;
  GtkWidget *Artist;
  GtkWidget *Title;
  GtkWidget *Album;
  GtkWidget *Year;
  GtkWidget *Encoded;
  GtkWidget *entry1;
  GtkWidget *labelYear;
  GtkWidget *labelEncoded;
  GtkWidget *text1;
  GtkWidget *entry2;
  GtkWidget *labelAlbum;
  GtkWidget *entry3;
  GtkWidget *labelTrack;
  GtkWidget *labelComposer;
  GtkWidget *label15;
  GtkWidget *hseparator1;
  GtkWidget *labelArtist;
  GtkWidget *labelTitle;
  GtkWidget *label17;
  GtkWidget *entry4;
  GtkWidget *label18;
  GtkWidget *labelGenre;
  GtkWidget *frame1;
  GtkWidget *fixed2;
  GtkWidget *labelaacType;
  GtkWidget *label13;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();
  updateWindowDatas();
  window1 = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_window_set_title (GTK_WINDOW (window1), "Infos / AAC / ID3tag");
  gtk_window_set_modal (GTK_WINDOW (window1), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window1), 400, 350);

  fixed1 = gtk_fixed_new ();
  gtk_widget_ref (fixed1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "fixed1", fixed1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed1);
  gtk_container_add (GTK_CONTAINER (window1), fixed1);

  button2 = gtk_button_new_with_label ("Close");
  gtk_widget_ref (button2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "button2", button2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_signal_connect_object(GTK_OBJECT(button2), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(window1));
  gtk_widget_show (button2);
  gtk_fixed_put (GTK_FIXED (fixed1), button2, 451, 39);
  gtk_widget_set_uposition (button2, 451, 39);
  gtk_widget_set_usize (button2, 49, 24);

  frame2 = gtk_frame_new ("ID3 Tag");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "frame2", frame2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_fixed_put (GTK_FIXED (fixed1), frame2, 2, 99);
  gtk_widget_set_uposition (frame2, 2, 99);
  gtk_widget_set_usize (frame2, 504, 326);

  fixed3 = gtk_fixed_new ();
  gtk_widget_ref (fixed3);
  gtk_object_set_data_full (GTK_OBJECT (window1), "fixed3", fixed3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed3);
  gtk_container_add (GTK_CONTAINER (frame2), fixed3);

  Track = gtk_entry_new ();
  gtk_widget_ref (Track);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Track", Track,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Track);
  gtk_fixed_put (GTK_FIXED (fixed3), Track, 424, 0);
  gtk_widget_set_uposition (Track, 424, 0);
  gtk_widget_set_usize (Track, 60, 20);
  gtk_tooltips_set_tip (tooltips, Track, "number on the album", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Track), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Track), track);

  Artist = gtk_entry_new ();
  gtk_widget_ref (Artist);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Artist", Artist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Artist);
  gtk_fixed_put (GTK_FIXED (fixed3), Artist, 112, 0);
  gtk_widget_set_uposition (Artist, 112, 0);
  gtk_widget_set_usize (Artist, 240, 20);
  gtk_tooltips_set_tip (tooltips, Artist, "artist or group singer", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Artist), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Artist), artist);

  Title = gtk_entry_new ();
  gtk_widget_ref (Title);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Title", Title,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Title);
  gtk_fixed_put (GTK_FIXED (fixed3), Title, 112, 32);
  gtk_widget_set_uposition (Title, 112, 32);
  gtk_widget_set_usize (Title, 370, 20);
  gtk_tooltips_set_tip (tooltips, Title, "title of the song", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Title), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Title), title);

  Album = gtk_entry_new ();
  gtk_widget_ref (Album);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Album", Album,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Album);
  gtk_fixed_put (GTK_FIXED (fixed3), Album, 112, 64);
  gtk_widget_set_uposition (Album, 112, 64);
  gtk_widget_set_usize (Album, 240, 20);
  gtk_tooltips_set_tip (tooltips, Album, "from album...", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Album), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Album), album);

  Year = gtk_entry_new_with_max_length (4);
  gtk_widget_ref (Year);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Year", Year,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Year);
  gtk_fixed_put (GTK_FIXED (fixed3), Year, 424, 64);
  gtk_widget_set_uposition (Year, 424, 64);
  gtk_widget_set_usize (Year, 60, 20);
  gtk_tooltips_set_tip (tooltips, Year, "sell in ...", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Year), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Year), year);

  Encoded = gtk_entry_new ();
  gtk_widget_ref (Encoded);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Encoded", Encoded,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Encoded);
  gtk_fixed_put (GTK_FIXED (fixed3), Encoded, 336, 272);
  gtk_widget_set_uposition (Encoded, 336, 272);
  gtk_widget_set_usize (Encoded, 158, 20);
  gtk_tooltips_set_tip (tooltips, Encoded, "the name of the encoder...", NULL);
  gtk_entry_set_editable (GTK_ENTRY (Encoded), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Encoded), encodedby);

  entry1 = gtk_entry_new ();
  gtk_widget_ref (entry1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "entry1", entry1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry1);
  gtk_fixed_put (GTK_FIXED (fixed3), entry1, 328, 96);
  gtk_widget_set_uposition (entry1, 328, 96);
  gtk_widget_set_usize (entry1, 160, 20);
  gtk_tooltips_set_tip (tooltips, entry1, "what king of music...", NULL);
  gtk_entry_set_editable (GTK_ENTRY (entry1), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry1), genre);

  labelYear = gtk_label_new ("Year :");
  gtk_widget_ref (labelYear);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelYear", labelYear,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelYear);
  gtk_fixed_put (GTK_FIXED (fixed3), labelYear, 376, 64);
  gtk_widget_set_uposition (labelYear, 376, 64);
  gtk_widget_set_usize (labelYear, 41, 18);

  labelEncoded = gtk_label_new ("Encoded by :");
  gtk_widget_ref (labelEncoded);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelEncoded", labelEncoded,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelEncoded);
  gtk_fixed_put (GTK_FIXED (fixed3), labelEncoded, 248, 272);
  gtk_widget_set_uposition (labelEncoded, 248, 272);
  gtk_widget_set_usize (labelEncoded, 83, 18);

  text1 = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "text1", text1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (text1);
  gtk_fixed_put (GTK_FIXED (fixed3), text1, 112, 152);
  gtk_widget_set_uposition (text1, 112, 152);
  gtk_widget_set_usize (text1, 376, 72);

  entry2 = gtk_entry_new ();
  gtk_widget_ref (entry2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "entry2", entry2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry2);
  gtk_fixed_put (GTK_FIXED (fixed3), entry2, 248, 232);
  gtk_widget_set_uposition (entry2, 248, 232);
  gtk_widget_set_usize (entry2, 240, 20);
  gtk_tooltips_set_tip (tooltips, entry2, "Composer of the song", NULL);
  gtk_entry_set_editable (GTK_ENTRY (entry2), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry2), composer);

  labelAlbum = gtk_label_new ("Album :");
  gtk_widget_ref (labelAlbum);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelAlbum", labelAlbum,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelAlbum);
  gtk_fixed_put (GTK_FIXED (fixed3), labelAlbum, 48, 64);
  gtk_widget_set_uposition (labelAlbum, 48, 64);
  gtk_widget_set_usize (labelAlbum, 55, 18);
  gtk_label_set_justify (GTK_LABEL (labelAlbum), GTK_JUSTIFY_RIGHT);

  entry3 = gtk_entry_new ();
  gtk_widget_ref (entry3);
  gtk_object_set_data_full (GTK_OBJECT (window1), "entry3", entry3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry3);
  gtk_fixed_put (GTK_FIXED (fixed3), entry3, 96, 272);
  gtk_widget_set_uposition (entry3, 96, 272);
  gtk_widget_set_usize (entry3, 145, 20);
  gtk_tooltips_set_tip (tooltips, entry3, "if a remix who's the original artist...", NULL);
  gtk_entry_set_editable (GTK_ENTRY (entry3), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry3), originalArtist);

  labelTrack = gtk_label_new ("Track :");
  gtk_widget_ref (labelTrack);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelTrack", labelTrack,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelTrack);
  gtk_fixed_put (GTK_FIXED (fixed3), labelTrack, 368, 0);
  gtk_widget_set_uposition (labelTrack, 368, 0);
  gtk_widget_set_usize (labelTrack, 47, 18);

  labelComposer = gtk_label_new ("Composer :");
  gtk_widget_ref (labelComposer);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelComposer", labelComposer,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelComposer);
  gtk_fixed_put (GTK_FIXED (fixed3), labelComposer, 160, 232);
  gtk_widget_set_uposition (labelComposer, 160, 232);
  gtk_widget_set_usize (labelComposer, 78, 18);

  label15 = gtk_label_new ("Comment :");
  gtk_widget_ref (label15);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label15", label15,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label15);
  gtk_fixed_put (GTK_FIXED (fixed3), label15, 32, 160);
  gtk_widget_set_uposition (label15, 32, 160);
  gtk_widget_set_usize (label15, 72, 18);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_fixed_put (GTK_FIXED (fixed3), hseparator1, 144, 120);
  gtk_widget_set_uposition (hseparator1, 144, 120);
  gtk_widget_set_usize (hseparator1, 278, 16);

  labelArtist = gtk_label_new ("Artist / Group :");
  gtk_widget_ref (labelArtist);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelArtist", labelArtist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelArtist);
  gtk_fixed_put (GTK_FIXED (fixed3), labelArtist, 0, 0);
  gtk_widget_set_uposition (labelArtist, 0, 0);
  gtk_widget_set_usize (labelArtist, 106, 18);
  gtk_label_set_justify (GTK_LABEL (labelArtist), GTK_JUSTIFY_RIGHT);

  labelTitle = gtk_label_new ("Title :");
  gtk_widget_ref (labelTitle);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelTitle", labelTitle,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelTitle);
  gtk_fixed_put (GTK_FIXED (fixed3), labelTitle, 62, 36);
  gtk_widget_set_uposition (labelTitle, 62, 36);
  gtk_widget_set_usize (labelTitle, 39, 18);

  label17 = gtk_label_new ("Original Artist :");
  gtk_widget_ref (label17);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label17", label17,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label17);
  gtk_fixed_put (GTK_FIXED (fixed3), label17, 0, 272);
  gtk_widget_set_uposition (label17, 0, 272);
  gtk_widget_set_usize (label17, 95, 18);

  entry4 = gtk_entry_new ();
  gtk_widget_ref (entry4);
  gtk_object_set_data_full (GTK_OBJECT (window1), "entry4", entry4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry4);
  gtk_fixed_put (GTK_FIXED (fixed3), entry4, 96, 96);
  gtk_widget_set_uposition (entry4, 96, 96);
  gtk_widget_set_usize (entry4, 158, 20);
  gtk_tooltips_set_tip (tooltips, entry4, "Artist Web site", NULL);
  gtk_entry_set_text (GTK_ENTRY (entry4), url);

  label18 = gtk_label_new ("Web site :");
  gtk_widget_ref (label18);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label18", label18,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label18);
  gtk_fixed_put (GTK_FIXED (fixed3), label18, 24, 96);
  gtk_widget_set_uposition (label18, 24, 96);
  gtk_widget_set_usize (label18, 67, 18);

  labelGenre = gtk_label_new ("Genre :");
  gtk_widget_ref (labelGenre);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelGenre", labelGenre,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelGenre);
  gtk_fixed_put (GTK_FIXED (fixed3), labelGenre, 272, 96);
  gtk_widget_set_uposition (labelGenre, 272, 96);
  gtk_widget_set_usize (labelGenre, 46, 18);

  frame1 = gtk_frame_new ("Infos");
  gtk_widget_ref (frame1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "frame1", frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame1);
  gtk_fixed_put (GTK_FIXED (fixed1), frame1, 5, 2);
  gtk_widget_set_uposition (frame1, 5, 2);
  gtk_widget_set_usize (frame1, 436, 96);

  fixed2 = gtk_fixed_new ();
  gtk_widget_ref (fixed2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "fixed2", fixed2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed2);
  gtk_container_add (GTK_CONTAINER (frame1), fixed2);

  labelaacType = gtk_label_new ("MPEG Type :");
  gtk_widget_ref (labelaacType);
  gtk_object_set_data_full (GTK_OBJECT (window1), "labelaacType", labelaacType,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (labelaacType);
  gtk_fixed_put (GTK_FIXED (fixed2), labelaacType, 32, 16);
  gtk_widget_set_uposition (labelaacType, 32, 16);
  gtk_widget_set_usize (labelaacType, 85, 18);

  label13 = gtk_label_new ("Frame Type :");
  gtk_widget_ref (label13);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label13", label13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label13);
  gtk_fixed_put (GTK_FIXED (fixed2), label13, 32, 40);
  gtk_widget_set_uposition (label13, 32, 40);
  gtk_widget_set_usize (label13, 85, 18);

  gtk_object_set_data (GTK_OBJECT (window1), "tooltips", tooltips);

  return window1;
}

// to don't have Gtk errors...
void updateWindowDatas(void)
{
 if(!title)
	title="";
 if(!artist)
	artist="";
 if(!album)
	album="";
 if(!year)
	year="";
 if(!track)
	track="";
 if(!genre)
	genre="";
 if(!comment)
	comment="";
 if(!composer)
	composer="";
 if(!url)
	url="";
 if(!originalArtist)
	originalArtist="";
 if(!encodedby)
	encodedby="";
}
