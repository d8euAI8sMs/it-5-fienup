
// testDlg.cpp : implementation file
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"
#include "afxdialogex.h"
#include "plot.h"

#include <iostream>
#include <array>
#include <numeric>
#include <atomic>
#include <thread>
#include <ctime>

#include "fft.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATEDATA_MESSAGE (WM_USER + 1000)

using namespace common;

namespace
{

    simple_list_plot
        signal_plot                 = simple_list_plot::connected_points(0, 2, 2),
        restored_plot               = simple_list_plot::connected_points(RGB(180, 0, 0), 4, 2),
        spectrum_magnitude_plot     = simple_list_plot::curve(0, 1);

    std::atomic<bool> working;
}

CtestDlg::CtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestDlg::IDD, pParent)
    , sample_count(512)
    , accuracy(1e-6)
    , max_steps(100000)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    signal_restored_signal_plot_ctrl.plot_layer.with(
        ((plot::plot_builder() << restored_plot) << signal_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
    );
    spectrum_magnitude_plot_ctrl.plot_layer.with(
        (plot::plot_builder() << spectrum_magnitude_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
        );

    input_params[0] = { 1, 5,  60 };
    input_params[1] = { 2, 8,  100 };
    input_params[2] = { 3, 10, 200 };
    input_params[3] = { 2, 13, 250 };
    input_params[4] = { 1, 18, 400 };
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PLOT_SIGNALS, signal_restored_signal_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_SPECTRUM, spectrum_magnitude_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_CORRELATION, correlation_plot_ctrl);
    DDX_Control(pDX, IDC_PLOT_SPECTRUM_PHASE, spectrum_phase_plot_ctrl);
    DDX_Text(pDX, IDC_SAMPLE_COUNT, sample_count);
    DDX_Text(pDX, IDC_SNR, snr_percents);
    DDX_Text(pDX, IDC_REL_ERROR, error);
    DDX_Text(pDX, IDC_REL_ERROR_ACC, acc_error);
    DDX_Text(pDX, IDC_LIMITS_ACC, accuracy);
    DDX_Text(pDX, IDC_LIMITS_STEPS, max_steps);
    DDX_Text(pDX, IDC_SIGNAL_A1, input_params[0].a);
    DDX_Text(pDX, IDC_SIGNAL_A2, input_params[1].a);
    DDX_Text(pDX, IDC_SIGNAL_A3, input_params[2].a);
    DDX_Text(pDX, IDC_SIGNAL_A44, input_params[3].a);
    DDX_Text(pDX, IDC_SIGNAL_A5, input_params[4].a);
    DDX_Text(pDX, IDC_SIGNAL_S1, input_params[0].s);
    DDX_Text(pDX, IDC_SIGNAL_S2, input_params[1].s);
    DDX_Text(pDX, IDC_SIGNAL_S3, input_params[2].s);
    DDX_Text(pDX, IDC_SIGNAL_S44, input_params[3].s);
    DDX_Text(pDX, IDC_SIGNAL_S5, input_params[4].s);
    DDX_Text(pDX, IDC_SIGNAL_t1, input_params[0].t0);
    DDX_Text(pDX, IDC_SIGNAL_t2, input_params[1].t0);
    DDX_Text(pDX, IDC_SIGNAL_t3, input_params[2].t0);
    DDX_Text(pDX, IDC_SIGNAL_t44, input_params[3].t0);
    DDX_Text(pDX, IDC_SIGNAL_t5, input_params[4].t0);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_START_PAUSE, &CtestDlg::OnBnClickedStartPause)
    ON_BN_CLICKED(IDC_STOP, &CtestDlg::OnBnClickedStop)
    ON_MESSAGE(WM_UPDATEDATA_MESSAGE, &CtestDlg::OnUpdateDataMessage)
END_MESSAGE_MAP()


// CtestDlg message handlers

BOOL CtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    UpdateData(FALSE);
    UpdateData(TRUE);

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CtestDlg::OnPaint()
{
	CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestDlg::OnBnClickedStartPause()
{
    bool expected = false;
    if (!working.compare_exchange_strong(expected, true))
    {
        return;
    }
    std::thread worker(&CtestDlg::DoWork, this);
    worker.detach();
}


void CtestDlg::OnBnClickedStop()
{
    working = false;
}


void CtestDlg::DoWork()
{
    DoWork0();
}


// void CtestDlg::DoWork0()
// {
//     RequestUpdateData(TRUE);
// 
//     srand(clock() % UINT_MAX);
// 
//     // setup periods and bounds
// 
//     sampled_t input_sampled = allocate_sampled(sample_count, 1);
//     sampled_t output_sampled = allocate_sampled(sample_count, 1);
//     sampled_t correlation_sampled = allocate_sampled(sample_count, 1);
//     sampled_t correlation_reflected_sampled = allocate_sampled(sample_count, 1);
//     sampled_t spectrum_magnitude_sampled = allocate_sampled(sample_count, 1);
//     sampled_t spectrum_phase_sampled = allocate_sampled(sample_count, 1);
//     sampled_t spectrum_restored_phase_sampled = allocate_sampled(sample_count, 1);
//     sampled_t noise_sampled = allocate_sampled(sample_count, 1);
//     sampled_t tmp = allocate_sampled(sample_count, 1);
//     cmplx    *fft = new cmplx[sample_count];
// 
//     // create signal
// 
//     continuous_t input_signals[] = {
//         gaussian(input_params[0]),
//         gaussian(input_params[1]),
//         gaussian(input_params[2]),
//         gaussian(input_params[3]),
//         gaussian(input_params[4])
//     };
//     sample(combine(5, input_signals), input_sampled);
// 
//     setup(signal_plot, input_sampled);
//     signal_restored_signal_plot_ctrl.RedrawWindow();
// 
//     // calculate fourier
// 
//     for (size_t i = 0; i < sample_count; i++)
//     {
//         fft[i].real = input_sampled.samples[i];
//         fft[i].image = 0;
//     }
// 
//     fourier(fft, sample_count, -1);
// 
//     double last_phase = 0; double theta = M_PI / 6;
//     for (size_t i = 0; i < sample_count; i++)
//     {
//         double random_phase = M_PI - (((double) rand() / RAND_MAX) * 2 * M_PI);
//         double magnitude = std::sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
//         spectrum_magnitude_sampled.samples[i] = magnitude;
//         //         spectrum_phase_sampled.samples[i] = std::atan2(fft[i].image, fft[i].real);
//         //         if (spectrum_phase_sampled.samples[i] - last_phase > M_PI - theta)
//         //         {
//         //             spectrum_phase_sampled.samples[i] -= M_PI;
//         //         }
//         //         else if (last_phase - spectrum_phase_sampled.samples[i] > M_PI - theta)
//         //         {
//         //             spectrum_phase_sampled.samples[i] += M_PI;
//         //         }
//         //         last_phase = spectrum_phase_sampled.samples[i];
// 
//         fft[i].real = magnitude * std::cos(random_phase);
//         fft[i].image = magnitude * std::sin(random_phase);
// 
//         //         spectrum_restored_phase_sampled.samples[i] = random_phase;
//     }
// 
//     setup(spectrum_magnitude_plot, spectrum_magnitude_sampled);
//     //     setup(spectrum_phase_plot, spectrum_phase_sampled);
//     //     setup(spectrum_restored_phase_plot, spectrum_restored_phase_sampled);
//     spectrum_magnitude_plot_ctrl.RedrawWindow();
//     //     spectrum_phase_plot_ctrl.RedrawWindow();
// 
//     // fienup algorithm
// 
//     while (working)
//     {
//         // inverse fourier -> back to signal
// 
//         fourier(fft, sample_count, 1);
// 
//         // get real part of a signal
//         // it may have a cyclic shift so we need to shift it back
//         // to display correctly (don't want to deal with reflection,
//         // just ignore)
// 
//         for (size_t i = 0; i < sample_count; i++)
//         {
//             output_sampled.samples[i] = fft[i].real;
//             tmp.samples[i] = fft[i].real;
//             fft[i].real = max(fft[i].real, 0);
//             fft[i].image = 0;
//         }
// 
//         // calculate correlation to determine a value of cyclic shift to perform
// 
//         auto max_reflected = correlation(input_sampled, output_sampled, correlation_reflected_sampled, true);
//         auto max = correlation(input_sampled, output_sampled, correlation_sampled);
//         setup(correlation_plot, correlation_sampled);
//         setup(correlation_reflected_plot, correlation_reflected_sampled);
//         correlation_plot_ctrl.RedrawWindow();
// 
//         size_t shift = max.first; bool reflected = max_reflected.second > max.second;
//         if (reflected) shift = max_reflected.first;
// 
//         // do a cyclic shift and draw the restored signal
// 
//         for (size_t i = 0; i < sample_count; i++)
//         {
//             size_t idx = (reflected ? sample_count - i - 1 : i);
//             output_sampled.samples[i] = tmp.samples[(shift + idx) % sample_count];
//         }
//         setup(restored_plot, output_sampled);
//         signal_restored_signal_plot_ctrl.RedrawWindow();
// 
//         // back to fourier
// 
//         fourier(fft, sample_count, -1);
//         for (size_t i = 0; i < sample_count; i++)
//         {
//             double magnitude = std::sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
//             fft[i].real /= magnitude;
//             fft[i].image /= magnitude;
//             //             spectrum_restored_phase_sampled.samples[i] = std::atan2(fft[i].image, fft[i].real);
//             //             if (spectrum_restored_phase_sampled.samples[i] - last_phase > M_PI - theta)
//             //             {
//             //                 spectrum_restored_phase_sampled.samples[i] -= M_PI;
//             //             }
//             //             else if (last_phase - spectrum_restored_phase_sampled.samples[i] > M_PI - theta)
//             //             {
//             //                 spectrum_restored_phase_sampled.samples[i] += M_PI;
//             //             }
//             //             last_phase = spectrum_restored_phase_sampled.samples[i];
// 
//             fft[i].real *= spectrum_magnitude_sampled.samples[i];
//             fft[i].image *= spectrum_magnitude_sampled.samples[i];
//         }
//         //         setup(spectrum_restored_phase_plot, spectrum_restored_phase_sampled);
//         //         spectrum_phase_plot_ctrl.RedrawWindow();
//     }
// 
//     // free allocated memory
// 
//     free_sampled(noise_sampled);
//     free_sampled(input_sampled);
//     free_sampled(spectrum_magnitude_sampled);
//     free_sampled(spectrum_phase_sampled);
//     free_sampled(spectrum_restored_phase_sampled);
//     free_sampled(correlation_sampled);
//     free_sampled(correlation_reflected_sampled);
//     free_sampled(tmp);
//     delete[] fft;
// 
//     working = false;
// }

void CtestDlg::DoWork0()
{
    RequestUpdateData(TRUE);
    
    srand(clock() % UINT_MAX);

    // setup periods and bounds

    sampled_t input_sampled = allocate_sampled(sample_count, 1);
    sampled_t output_sampled = allocate_sampled(sample_count, 1);
    sampled_t prev_output_sampled = allocate_sampled(sample_count, 1);
    sampled_t correlation_sampled = allocate_sampled(sample_count, 1);
    sampled_t correlation_reflected_sampled = allocate_sampled(sample_count, 1);
    sampled_t spectrum_magnitude_sampled = allocate_sampled(sample_count, 1);
    sampled_t noise_sampled = allocate_sampled(sample_count, 1);
    sampled_t tmp = allocate_sampled(sample_count, 1);
    cmplx    *fft = new cmplx[sample_count];

    // create signal

    continuous_t input_signals[] = {
        gaussian(input_params[0]),
        gaussian(input_params[1]),
        gaussian(input_params[2]),
        gaussian(input_params[3]),
        gaussian(input_params[4])
    };
    sample(combine(5, input_signals), input_sampled);

    setup(signal_plot, input_sampled);
    signal_restored_signal_plot_ctrl.RedrawWindow();

    // calculate fourier

    for (size_t i = 0; i < sample_count; i++)
    {
        fft[i].real = input_sampled.samples[i];
        fft[i].image = 0;
    }

    fourier(fft, sample_count, -1);

    double last_phase = 0; double theta = M_PI / 6;
    for (size_t i = 0; i < sample_count; i++)
    {
        double random_phase = M_PI - (((double) rand() / RAND_MAX) * 2 * M_PI);
        double magnitude = std::sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
        spectrum_magnitude_sampled.samples[i] = magnitude;
        fft[i].real = magnitude * std::cos(random_phase);
        fft[i].image = magnitude * std::sin(random_phase);
    }

    setup(spectrum_magnitude_plot, spectrum_magnitude_sampled);
    spectrum_magnitude_plot_ctrl.RedrawWindow();

    // fienup algorithm

    size_t counter = 0;
    size_t shift = 0;
    bool reflected = false;
    bool need_stop = false;
    while(true)
    {
        // inverse fourier -> back to signal

        fourier(fft, sample_count, 1);

        // get real part of a signal
        // it may have a cyclic shift so we need to shift it back
        // to display correctly (don't want to deal with reflection,
        // just ignore)

        for (size_t i = 0; i < sample_count; i++)
        {
            tmp.samples[i] = fft[i].real;
            fft[i].real = max(fft[i].real, 0);
            fft[i].image = 0;
        }

        // calculate correlation to determine a value of cyclic shift to perform

        if ((counter++ % 100) == 0 || need_stop) // do an initial shift and then apply corrections
        {
            auto max_reflected = correlation(input_sampled, tmp, correlation_reflected_sampled, true);
            auto max = correlation(input_sampled, tmp, correlation_sampled);

            shift = max.first; reflected = max_reflected.second > max.second;
            if (reflected) shift = max_reflected.first;
        }

        for (size_t i = 0; i < sample_count; i++)
        {
            size_t idx = (reflected ? sample_count - i - 1 : i);
            output_sampled.samples[i] = tmp.samples[(shift + idx) % sample_count];
        }
        setup(restored_plot, output_sampled);
        signal_restored_signal_plot_ctrl.RedrawWindow();

        // calculate error

        error = 0;
        for (size_t i = 0; i < sample_count; i++)
        {
            error += (output_sampled.samples[i] - input_sampled.samples[i])
                * (output_sampled.samples[i] - input_sampled.samples[i]);
        }
        error /= sample_count;
        error = std::sqrt(error);

        RequestUpdateData(FALSE);

        // back to fourier

        fourier(fft, sample_count, -1);
        for (size_t i = 0; i < sample_count; i++)
        {
            double magnitude = std::sqrt(fft[i].real * fft[i].real + fft[i].image * fft[i].image);
            if (magnitude < 1e-12) continue;
            fft[i].real *= spectrum_magnitude_sampled.samples[i] / magnitude;
            fft[i].image *= spectrum_magnitude_sampled.samples[i] / magnitude;
        }

        // calculate iteration differences

        if (need_stop) break; // lazy stop

        acc_error = 0;
        for (size_t i = 0; i < sample_count; i++)
        {
            // use unshifted signal
            acc_error += (tmp.samples[i] - prev_output_sampled.samples[i])
                * (tmp.samples[i] - prev_output_sampled.samples[i]);
        }

        acc_error /= sample_count;

        RequestUpdateData(FALSE);

        if (acc_error < accuracy || !working) need_stop = true;

        map(prev_output_sampled, tmp); // use unshifted signal
    }

    // free allocated memory

    free_sampled(noise_sampled);
    free_sampled(input_sampled);
    free_sampled(spectrum_magnitude_sampled);
    free_sampled(correlation_sampled);
    free_sampled(correlation_reflected_sampled);
    free_sampled(tmp);
    free_sampled(output_sampled);
    free_sampled(prev_output_sampled);
    delete[] fft;

    working = false;
}


LRESULT CtestDlg::OnUpdateDataMessage(WPARAM wpD, LPARAM lpD)
{
    UpdateData(wpD == TRUE);
    return 0;
}

void CtestDlg::RequestUpdateData(BOOL saveAndValidate)
{
    SendMessage(WM_UPDATEDATA_MESSAGE, saveAndValidate);
}