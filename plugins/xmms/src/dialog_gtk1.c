
#include <gtk/gtk.h>

char *title=0;
char *artist=0;
char *album=0;
char *year=0;
char *track=0;
char *genre=0;
char *comment=0;
char *composer=0;
char *url=0;
char *originalArtist=0;
char *encodedby=0;

void updateWindowDatas(void);
GtkWidget* createDialogInfo(void);

GtkWidget* createDialogInfo(void)

{
  GtkWidget *window1;
  GtkWidget *fixed1;
  GtkWidget *button1;
  GtkWidget *AACTypeentry;
  GtkWidget *HeaderTypeentry;
  GtkWidget *frame1;
  GtkWidget *fixed2;
  GtkWidget *Titleentry;
  GtkWidget *Artistentry;
  GtkWidget *Trackentry;
  GtkWidget *Albumentry;
  GtkWidget *Yearentry;
  GtkWidget *CommentText;
  GtkWidget *Composerentry;
  GtkWidget *label9;
  GtkWidget *label8;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label5;
  GtkWidget *OrArtistentry;
  GtkWidget *label10;
  GtkWidget *Encodedentry;
  GtkWidget *label11;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkTooltips *tooltips;

  updateWindowDatas();
  tooltips = gtk_tooltips_new ();

  window1 = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_window_set_title (GTK_WINDOW (window1), "AAC info");

  fixed1 = gtk_fixed_new ();
  gtk_widget_ref (fixed1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "fixed1", fixed1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed1);
  gtk_container_add (GTK_CONTAINER (window1), fixed1);

  button1 = gtk_button_new_with_label ("Close");
  gtk_widget_ref (button1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "button1", button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_signal_connect_object(GTK_OBJECT(button1), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(window1));
  gtk_widget_show (button1);
  gtk_fixed_put (GTK_FIXED (fixed1), button1, 408, 16);
  gtk_widget_set_uposition (button1, 408, 16);
  gtk_widget_set_usize (button1, 47, 22);

  AACTypeentry = gtk_entry_new ();
  gtk_widget_ref (AACTypeentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "AACTypeentry", AACTypeentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AACTypeentry);
  gtk_fixed_put (GTK_FIXED (fixed1), AACTypeentry, 128, 8);
  gtk_widget_set_uposition (AACTypeentry, 128, 8);
  gtk_widget_set_usize (AACTypeentry, 96, 16);
  gtk_entry_set_editable (GTK_ENTRY (AACTypeentry), FALSE);

  HeaderTypeentry = gtk_entry_new ();
  gtk_widget_ref (HeaderTypeentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "HeaderTypeentry", HeaderTypeentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (HeaderTypeentry);
  gtk_fixed_put (GTK_FIXED (fixed1), HeaderTypeentry, 128, 32);
  gtk_widget_set_uposition (HeaderTypeentry, 128, 32);
  gtk_widget_set_usize (HeaderTypeentry, 96, 16);
  gtk_entry_set_editable (GTK_ENTRY (HeaderTypeentry), FALSE);

  frame1 = gtk_frame_new ("ID3v2 Tag");
  gtk_widget_ref (frame1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "frame1", frame1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame1);
  gtk_fixed_put (GTK_FIXED (fixed1), frame1, 8, 64);
  gtk_widget_set_uposition (frame1, 8, 64);
  gtk_widget_set_usize (frame1, 464, 192);

  fixed2 = gtk_fixed_new ();
  gtk_widget_ref (fixed2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "fixed2", fixed2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (fixed2);
  gtk_container_add (GTK_CONTAINER (frame1), fixed2);

  Titleentry = gtk_entry_new ();
  gtk_widget_ref (Titleentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Titleentry", Titleentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Titleentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Titleentry, 80, 0);
  gtk_widget_set_uposition (Titleentry, 80, 0);
  gtk_widget_set_usize (Titleentry, 232, 16);
  gtk_entry_set_editable (GTK_ENTRY (Titleentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Titleentry), title);

  Artistentry = gtk_entry_new ();
  gtk_widget_ref (Artistentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Artistentry", Artistentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Artistentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Artistentry, 80, 16);
  gtk_widget_set_uposition (Artistentry, 80, 16);
  gtk_widget_set_usize (Artistentry, 232, 16);
  gtk_entry_set_editable (GTK_ENTRY (Artistentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Artistentry), artist);

  Trackentry = gtk_entry_new ();
  gtk_widget_ref (Trackentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Trackentry", Trackentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Trackentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Trackentry, 400, 32);
  gtk_widget_set_uposition (Trackentry, 400, 32);
  gtk_widget_set_usize (Trackentry, 56, 16);
  gtk_entry_set_editable (GTK_ENTRY (Trackentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Trackentry), track);

  Albumentry = gtk_entry_new ();
  gtk_widget_ref (Albumentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Albumentry", Albumentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Albumentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Albumentry, 80, 32);
  gtk_widget_set_uposition (Albumentry, 80, 32);
  gtk_widget_set_usize (Albumentry, 232, 16);
  gtk_entry_set_editable (GTK_ENTRY (Albumentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Albumentry), album);

  Yearentry = gtk_entry_new ();
  gtk_widget_ref (Yearentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Yearentry", Yearentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Yearentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Yearentry, 384, 0);
  gtk_widget_set_uposition (Yearentry, 384, 0);
  gtk_widget_set_usize (Yearentry, 72, 16);
  gtk_entry_set_editable (GTK_ENTRY (Yearentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Yearentry), year);

  CommentText = gtk_text_new (NULL, NULL);
  gtk_widget_ref (CommentText);
  gtk_object_set_data_full (GTK_OBJECT (window1), "CommentText", CommentText,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (CommentText);
  gtk_fixed_put (GTK_FIXED (fixed2), CommentText, 80, 56);
  gtk_widget_set_uposition (CommentText, 80, 56);
  gtk_widget_set_usize (CommentText, 376, 48);

  Composerentry = gtk_entry_new ();
  gtk_widget_ref (Composerentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Composerentry", Composerentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Composerentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Composerentry, 80, 112);
  gtk_widget_set_uposition (Composerentry, 80, 112);
  gtk_widget_set_usize (Composerentry, 232, 16);
  gtk_entry_set_editable (GTK_ENTRY (Composerentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Composerentry), composer);

  label9 = gtk_label_new ("Composer :");
  gtk_widget_ref (label9);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label9", label9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label9);
  gtk_fixed_put (GTK_FIXED (fixed2), label9, 0, 112);
  gtk_widget_set_uposition (label9, 0, 112);
  gtk_widget_set_usize (label9, 80, 16);
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_RIGHT);

  label8 = gtk_label_new ("Comment :");
  gtk_widget_ref (label8);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label8", label8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label8);
  gtk_fixed_put (GTK_FIXED (fixed2), label8, 0, 72);
  gtk_widget_set_uposition (label8, 0, 72);
  gtk_widget_set_usize (label8, 72, 16);
  gtk_label_set_justify (GTK_LABEL (label8), GTK_JUSTIFY_RIGHT);

  label3 = gtk_label_new ("Title :");
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_fixed_put (GTK_FIXED (fixed2), label3, 8, 0);
  gtk_widget_set_uposition (label3, 8, 0);
  gtk_widget_set_usize (label3, 56, 16);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_RIGHT);

  label4 = gtk_label_new ("Artist :");
  gtk_widget_ref (label4);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label4", label4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label4);
  gtk_fixed_put (GTK_FIXED (fixed2), label4, 8, 16);
  gtk_widget_set_uposition (label4, 8, 16);
  gtk_widget_set_usize (label4, 56, 16);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_RIGHT);

  label6 = gtk_label_new ("Album :");
  gtk_widget_ref (label6);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label6", label6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label6);
  gtk_fixed_put (GTK_FIXED (fixed2), label6, 8, 32);
  gtk_widget_set_uposition (label6, 8, 32);
  gtk_widget_set_usize (label6, 48, 16);
  gtk_label_set_justify (GTK_LABEL (label6), GTK_JUSTIFY_RIGHT);

  label7 = gtk_label_new ("Year :");
  gtk_widget_ref (label7);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label7", label7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label7);
  gtk_fixed_put (GTK_FIXED (fixed2), label7, 328, 0);
  gtk_widget_set_uposition (label7, 328, 0);
  gtk_widget_set_usize (label7, 64, 16);
  gtk_label_set_justify (GTK_LABEL (label7), GTK_JUSTIFY_RIGHT);

  label5 = gtk_label_new ("Track N\260 :");
  gtk_widget_ref (label5);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label5", label5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label5);
  gtk_fixed_put (GTK_FIXED (fixed2), label5, 328, 32);
  gtk_widget_set_uposition (label5, 328, 32);
  gtk_widget_set_usize (label5, 64, 16);
  gtk_label_set_justify (GTK_LABEL (label5), GTK_JUSTIFY_RIGHT);

  OrArtistentry = gtk_entry_new ();
  gtk_widget_ref (OrArtistentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "OrArtistentry", OrArtistentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (OrArtistentry);
  gtk_fixed_put (GTK_FIXED (fixed2), OrArtistentry, 80, 128);
  gtk_widget_set_uposition (OrArtistentry, 80, 128);
  gtk_widget_set_usize (OrArtistentry, 232, 16);
  gtk_tooltips_set_tip (tooltips, OrArtistentry, "Original Artist", NULL);
  gtk_entry_set_editable (GTK_ENTRY (OrArtistentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (OrArtistentry), originalArtist);

  label10 = gtk_label_new ("Or. Artist :");
  gtk_widget_ref (label10);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label10", label10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label10);
  gtk_fixed_put (GTK_FIXED (fixed2), label10, 0, 128);
  gtk_widget_set_uposition (label10, 0, 128);
  gtk_widget_set_usize (label10, 72, 16);

  Encodedentry = gtk_entry_new ();
  gtk_widget_ref (Encodedentry);
  gtk_object_set_data_full (GTK_OBJECT (window1), "Encodedentry", Encodedentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Encodedentry);
  gtk_fixed_put (GTK_FIXED (fixed2), Encodedentry, 112, 144);
  gtk_widget_set_uposition (Encodedentry, 112, 144);
  gtk_widget_set_usize (Encodedentry, 200, 16);
  gtk_entry_set_editable (GTK_ENTRY (Encodedentry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (Encodedentry), encodedby);

  label11 = gtk_label_new ("Encoded by :");
  gtk_widget_ref (label11);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label11", label11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label11);
  gtk_fixed_put (GTK_FIXED (fixed2), label11, 0, 144);
  gtk_widget_set_uposition (label11, 0, 144);
  gtk_widget_set_usize (label11, 104, 16);
  gtk_label_set_justify (GTK_LABEL (label11), GTK_JUSTIFY_RIGHT);

  label1 = gtk_label_new ("AAC Type :");
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_fixed_put (GTK_FIXED (fixed1), label1, 8, 8);
  gtk_widget_set_uposition (label1, 8, 8);
  gtk_widget_set_usize (label1, 112, 16);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_RIGHT);

  label2 = gtk_label_new ("Header Type :");
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_fixed_put (GTK_FIXED (fixed1), label2, 8, 32);
  gtk_widget_set_uposition (label2, 8, 32);
  gtk_widget_set_usize (label2, 112, 16);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_RIGHT);

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
