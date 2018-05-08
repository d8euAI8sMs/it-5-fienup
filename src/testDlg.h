
// testDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "common.h"

#include "PlotStatic.h"

// CtestDlg dialog
class CtestDlg : public CDialog
{
// Construction
public:
	CtestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CWnd plot;
    afx_msg void OnBnClickedStartPause();
    afx_msg void OnBnClickedStop();
    PlotStatic signal_restored_signal_plot_ctrl;
    PlotStatic spectrum_magnitude_plot_ctrl;
    PlotStatic spectrum_phase_plot_ctrl;
    PlotStatic correlation_plot_ctrl;
    size_t sample_count;
    double snr_percents;
    double error;
    double acc_error;
    double accuracy;
    int max_steps;
    common::gaussian_t input_params[5];
    void DoWork();
    LRESULT OnUpdateDataMessage(WPARAM wpD, LPARAM lpD);
    void RequestUpdateData(BOOL saveAndValidate);
    void DoWork0();
};
