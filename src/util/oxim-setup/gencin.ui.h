/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/



void GenCin::defaultButton_clicked()
{
    settings_t *settings = oxim_get_default_settings(imname, false);
    if (!settings)
    {
	return;
    }
	int yesno;
	// AutoCompose
	if (oxim_setting_GetBoolean(settings, "AutoCompose", &yesno))
	{
	    AutoCompose->setChecked(yesno);
	}
	
	// AutoUpChar
	if (oxim_setting_GetBoolean(settings, "AutoUpChar", &yesno))
	{
	    AutoUpChar->setChecked(yesno);
	}
	
	// AutoFullUp
	if (oxim_setting_GetBoolean(settings, "AutoFullUp", &yesno))
	{
	    AutoFullUp->setChecked(yesno);
	}
	
	// SpaceAutoUp
	if (oxim_setting_GetBoolean(settings, "SpaceAutoUp", &yesno))
	{
	    SpaceAutoUp->setChecked(yesno);
	}
	
	// SelectKeyShift
	if (oxim_setting_GetBoolean(settings, "SelectKeyShift", &yesno))
	{
	    SelectKeyShift->setChecked(yesno);
	}
	
	// SpaceIgnore
	if (oxim_setting_GetBoolean(settings, "SpaceIgnore", &yesno))
	{
	    if (yesno)
		SpaceIgnoreYes->setChecked(true);
	    else
		SpaceIgnoreNo->setChecked(true);
	}

	// SpaceReset
	if (oxim_setting_GetBoolean(settings, "SpaceReset", &yesno))
	{
	    SpaceReset->setChecked(yesno);
	}
	
	// WildEnable
	if (oxim_setting_GetBoolean(settings, "WildEnable", &yesno))
	{
	    WildEnable->setChecked(yesno);
	}
	
	// 設定 EndKey
	if (oxim_setting_GetBoolean(settings, "EndKey", &yesno))
	{
	    EndKey->setChecked(yesno);
	}

	// 設定特殊字根
	char *spstroke;
	if (oxim_setting_GetString(settings, "DisableSelectList", &spstroke))
	{
	    if (strcasecmp(spstroke, "None") != 0)
		DisableSelectList->setText(spstroke);
	}
}
