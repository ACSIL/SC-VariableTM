#include "sierrachart.h"
SCDLLName("VARIABLE TRADE MANAGEMENT SC")

//enum MovAvgTypeEnum
//{
//    MOVAVGTYPE_EXPONENTIAL
//    , MOVAVGTYPE_LINEARREGRESSION
//    , MOVAVGTYPE_SIMPLE
//    , MOVAVGTYPE_WEIGHTED
//    , MOVAVGTYPE_WILDERS
//    , MOVAVGTYPE_SIMPLE_SKIP_ZEROS
//    , MOVAVGTYPE_SMOOTHED
//    , MOVAVGTYPE_NUMBER_OF_AVERAGES
//};

const char *ma_type(const int &i)
{
    switch (i)
    { 
        case 0: return "EXPONENTIAL";
            break;
        case 1: return "LINEARREGRESSION";
            break;
        case 2: return "SIMPLE";
            break; 
        case 3: return "WEIGHTED";
            break;
        case 4: return "WILDERS";
            break;
        case 5: return "SIMPLE_SKIP_ZEROS";
            break;
        case 6: return "SMOOTHED";
            break;
        case 7: return "NUMBER_OF_AVERAGES";
            break;
        default: 
            return "UNKNOWN";
    }
}
//after each entry, sends PT and SL (with OCO) in the distance deriveded from actual volatility (with rr 1:1)
SCSFExport scsf_VariableTradeManagement(SCStudyInterfaceRef sc)
{
    SCInputRef multiplicator = sc.Input[0];

    if (sc.SetDefaults)
    {
        sc.Input[0].Name = "Multiplicator";
        sc.Input[0].SetFloat(3.0f);
        sc.Input[1].Name = "ATR period";
        sc.Input[1].SetInt(10);
        sc.Input[2].Name = "MA Type";
        sc.Input[2].SetMovAvgType(MOVAVGTYPE_SIMPLE);
        sc.Input[3].Name = "Display ATR details";
        sc.Input[3].SetYesNo(FALSE);

        sc.GraphName = "Variable Trade Management";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;
        sc.FreeDLL = 1;
        return;
    }

    sc.SupportAttachedOrdersForTrading = true;
    sc.AllowOnlyOneTradePerBar = false;
    sc.MaximumPositionAllowed = 100;

    s_SCPositionData current_position;
    sc.GetTradePosition(current_position);


    //calculate atr value
    sc.ATR(sc.BaseDataIn, sc.Subgraph[0], sc.Input[1].GetInt(), sc.Input[2].GetMovAvgType());
	float atr_value = sc.Subgraph[0][sc.Index];
    float atr_tick_value = atr_value / sc.TickSize;
    if (sc.Input[3].GetYesNo())
    {
        s_UseTool atr;
	    atr.Clear();
	    atr.ChartNumber = sc.ChartNumber;
	    atr.DrawingType = DRAWING_TEXT;
	    atr.FontSize = 8;
	    atr.FontBold = false;
	    atr.AddMethod = UTAM_ADD_OR_ADJUST;
	    atr.UseRelativeVerticalValues = 1;
	    atr.BeginDateTime = 25;
	    atr.BeginValue = 100;
	    atr.Color = RGB(255, 255, 255);
	    atr.Region = 0;
	    atr.Text.Format("USING ATR %s (PER.:%i): CURRENT VALUE IS %2.2f WHICH IS %2.2f TICKS", ma_type(sc.Input[2].GetMovAvgType()), sc.Input[1].GetInt(), atr_value, atr_tick_value);
	    atr.LineNumber = 1;
        sc.UseTool(atr);
    }
    s_UseTool slpt;
    slpt.Clear();
    slpt.ChartNumber = sc.ChartNumber;
    slpt.DrawingType = DRAWING_TEXT;
    slpt.FontSize = 8;
    slpt.FontBold = false;
    slpt.AddMethod = UTAM_ADD_OR_ADJUST;
    slpt.UseRelativeVerticalValues = 1;
    slpt.BeginDateTime = 1;
    slpt.BeginValue = 96;
    slpt.Color = RGB(255, 255, 255);
    slpt.Region = 0;
    slpt.Text.Format("SL/PT: %0.0f ticks", round(atr_tick_value));
    slpt.LineNumber = 2;
    sc.UseTool(slpt);

   	//define exit prices for long
    double long_SL = current_position.AveragePrice - multiplicator.GetFloat() * atr_tick_value * sc.TickSize;
    double long_PT = current_position.AveragePrice + multiplicator.GetFloat() * atr_tick_value * sc.TickSize;
    s_SCNewOrder long_exit;
	long_exit.OrderType = SCT_ORDERTYPE_OCO_LIMIT_STOP;
	long_exit.Price1 = long_PT;
	long_exit.Price2 = long_SL;

	//define exit prices for short
	double short_SL = current_position.AveragePrice + multiplicator.GetFloat() * atr_tick_value * sc.TickSize;
	double short_PT = current_position.AveragePrice - multiplicator.GetFloat() * atr_tick_value * sc.TickSize;
	s_SCNewOrder short_exit;
	short_exit.OrderType = SCT_ORDERTYPE_OCO_LIMIT_STOP;
	short_exit.Price1 = short_PT;
	short_exit.Price2 = short_SL;

	//set sl and pt after each entry
	int &previous_qt_perzist = sc.GetPersistentInt(0);
	t_OrderQuantity32_64 qty{ current_position.PositionQuantity };
	if (previous_qt_perzist == 0 && current_position.PositionQuantity > 0) // from no possition to long possition
	{
		t_OrderQuantity32_64 succesful_entry = sc.BuyExit(long_exit);
		if (succesful_entry > 0)
		{
			previous_qt_perzist = static_cast<int>(current_position.PositionQuantity);
		}
	}
	else if (previous_qt_perzist == 0 && current_position.PositionQuantity < 0) // from no possition to short possition
	{
		t_OrderQuantity32_64 succesful_entry = sc.SellExit(short_exit);
		if (succesful_entry > 0)
		{
			previous_qt_perzist = static_cast<int>(current_position.PositionQuantity);
		}
	}
   	//reset the perzist
	if (current_position.PositionQuantity == 0) previous_qt_perzist = 0;
}


