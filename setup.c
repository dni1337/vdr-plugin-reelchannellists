#include "setup.h"

int zapAllFavChannels;

#ifdef REELVDR
cMenuReelChannelListSetup::cMenuReelChannelListSetup() : wantChOnOkTmp(Setup.WantChListOnOk)
#else
cMenuReelChannelListSetup::cMenuReelChannelListSetup() : wantChOnOkTmp(true)
#endif
{
    wantChOnOkModes.Append(strdup(tr("Channel Info")));
    wantChOnOkModes.Append(strdup(tr("Standard")));
    wantChOnOkModes.Append(strdup(tr("Favourites"))); // like kYellow

    Set();
}

void cMenuReelChannelListSetup::Set()
{
    Clear();
    newZapAllFavChannels = zapAllFavChannels;
#ifdef REELVDR
    wantChOnOkTmp = Setup.WantChListOnOk;
#endif

    /*
    Add(new cMenuEditBoolItem(tr("Zapping in favourites list"),
                              &newZapAllFavChannels,
                              tr("Current folder"),
                              tr("All folders")));
    */

    Add(new cMenuEditStraItem(tr("Ok key in TV Mode"), &wantChOnOkTmp,
                              wantChOnOkModes.Size(), &wantChOnOkModes.At(0)));
}

void cMenuReelChannelListSetup::Store()
{
    zapAllFavChannels = newZapAllFavChannels;
#ifdef REELVDR
    Setup.WantChListOnOk = wantChOnOkTmp;
#endif

    SetupStore("ZapAllFavChannels", zapAllFavChannels);
}

