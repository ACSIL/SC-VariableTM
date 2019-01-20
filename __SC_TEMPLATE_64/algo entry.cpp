#include "sierrachart.h"

//after first fill additional market orders will be send to open into full possition

SCSFExport scsf_HFTAlgoEntries(SCStudyInterfaceRef sc)
{
    if (sc.SetDefaults)
    {    
        sc.Input[4].Name = "Final volume";
        sc.Input[4].SetInt(25);

        sc.GraphName = "HFT algo entries";
        sc.GraphRegion = 0;  

        sc.AutoLoop = 1;
        sc.FreeDLL = 1; 
        return;
    }
     sc.AllowMultipleEntriesInSameDirection = true;
    sc.MaximumPositionAllowed = 100;
    sc.SupportAttachedOrdersForTrading = false;
    sc.AllowEntryWithWorkingOrders = true;
    sc.AllowOnlyOneTradePerBar = false;
    sc.MaintainTradeStatisticsAndTradesData = true;
   
    s_SCNewOrder NewOrder;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.OrderQuantity = 1;
  
    s_SCPositionData current_possition;
    sc.GetTradePosition(current_possition);

    int& previous_qty = sc.GetPersistentInt(0);
    if (previous_qty == 0 && current_possition.PositionQuantity > 0)
    {
        int i = 0;
        while (i < sc.Input[4].GetInt() -1)
        {
            sc.BuyEntry(NewOrder);
            ++i;
        }
    }
    else if (previous_qty == 0 && current_possition.PositionQuantity < 0)
    {
        int i = 0;
        while (i < sc.Input[4].GetInt() -1)
        {
            sc.SellEntry(NewOrder);
            ++i;
        }
    }
    previous_qty = static_cast<int>(current_possition.PositionQuantity);
}