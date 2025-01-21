import argparse
import subprocess
import time
import os
from time import sleep
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import plotly.express as px
import pandas as pd


def load_data(file_path):
    df = pd.read_csv(file_path, sep=";", index_col=False, header=None, names=["Execution", "TestType", "SetTime", "GetTime"])
    df["SetTime"] = pd.to_timedelta(df["SetTime"], unit="us")
    df["GetTime"] = pd.to_timedelta(df["GetTime"], unit="us")
    return df


def format_y_axis_labels(value):
    """
    Format y-axis labels dynamically based on the value range.
    - 's' for seconds.
    - 'ms' for milliseconds.
    - 'µs' for microseconds.
    """
    if value >= 1:
        return f"{value:.0f}s"  # Seconds
    elif value >= 1e-3:
        return f"{value * 1e3:.0f}ms"  # Milliseconds
    else:
        return f"{value * 1e6:.0f}µs"  # Microseconds

def table_formater(value):
    """
    Format y-axis labels dynamically based on the value range.
    - 's' for seconds.
    - 'ms' for milliseconds.
    - 'µs' for microseconds.
    """
    if value >= 1:
        return f"{value:.2f}s"  # Seconds
    elif value >= 1e-3:
        return f"{value * 1e3:.2f}ms"  # Milliseconds
    else:
        return f"{value * 1e6:.2f}µs"  # Microseconds

def generate_custom_ticks(y_min, y_max):
    """
    Generate tick values following the specified pattern:
    - Start with 1, 2, 4, 8
    - Repeat at orders of magnitude (10, 20, 40, 80; 100, 200, etc.)
    """
    ticks = []
    tick = 0
    last = 0
    scale = 1e-6  # Start with 1 microsecond in seconds
    while True:
        for multiplier in [1, 2, 4, 6, 8]:
            if (tick >= y_max):
                ticks.append(last)
                return ticks
            tick = multiplier * scale
            if y_min <= tick <= y_max:
                ticks.append(tick)
            if 0 < last <= y_min <= tick:
                ticks.append(last)
            last = tick
        scale *= 10

def plot_boxplots_with_custom_labels(df):
    """
    Create boxplots for SetTime and GetTime using Plotly.
    Dynamically format y-axis tick labels to include units (s, ms, µs).
    """
    # Convert Timedelta to total seconds for Plotly
    df["SetTimeSeconds"] = df["SetTime"].dt.total_seconds()
    df["GetTimeSeconds"] = df["GetTime"].dt.total_seconds()

    # Prepare data in long format for Plotly
    plot_data = pd.melt(
        df,
        id_vars=["Execution", "TestType"],
        value_vars=["SetTimeSeconds", "GetTimeSeconds"],
        var_name="TimeType",
        value_name="TimeInSeconds"
    )

    # Update the TimeType column to make labels clearer
    plot_data["TimeType"] = plot_data["TimeType"].replace({
        "SetTimeSeconds": "Set Time",
        "GetTimeSeconds": "Get Time"
    })

    # Generate a separate figure for each test type
    test_types = plot_data["TestType"].unique()
    for test_type in test_types:
        test_data = plot_data[plot_data["TestType"] == test_type]

        averages = (
            test_data.groupby(["Execution", "TimeType"])["TimeInSeconds"]
            .mean()
            .reset_index()
        )

        # Create the boxplot for this test type
        fig = px.box(
            test_data,
            x="Execution",               # Group by execution type
            y="TimeInSeconds",           # Time in seconds
            color="TimeType",            # Distinguish SetTime and GetTime
            title=f"Benchmark: {test_type} (2 Threads on parallel benchmarks)",
            labels={"TimeInSeconds": "Time", "Execution": "Execution Type"},
            boxmode="group"              # Group SetTime and GetTime in one chart
        )

        # Get tick values dynamically
        y_min = test_data["TimeInSeconds"].min()
        y_max = test_data["TimeInSeconds"].max()
        tickvals = generate_custom_ticks(y_min, y_max)

        # Apply the formatting function to the ticks
        ticktext = [format_y_axis_labels(val) for val in tickvals]

        '''
        # Add average markers
        for _, row in averages.iterrows():
            fig.add_scatter(
                x=[row["Execution"]],                # Execution type
                y=[row["TimeInSeconds"]],           # Average time
                mode="text",
                marker=dict(size=10, symbol="diamond", color="black"),
                text=[f"Avg: {format_y_axis_labels(row['TimeInSeconds'])}"],
                textposition="top center",
                name=f"Avg {row['TimeType']}",       # Legend entry for the average
                showlegend=False                    # Don't show legend for averages
            )
        '''

        # Update layout with custom tick labels
        fig.update_layout(
            yaxis=dict(
                tickvals=tickvals,  # Tick positions
                ticktext=ticktext,  # Custom labels with units
                title="Time",
                type="log"
            ),
            xaxis_title="Executable",
            legend_title="Time Type",
            boxmode="group",
        )

        # Show the figure
        fig.show()

def generate_tables(df):
    df["SetTimeSeconds"] = df["SetTime"].dt.total_seconds()
    df["GetTimeSeconds"] = df["GetTime"].dt.total_seconds()

    markdown_tables = []

    # Separate single-threaded and parallel executables
    single_threaded = df[~df["Execution"].str.endswith("par")]
    parallel = df[df["Execution"].str.endswith("par")]

    # Helper function to generate the summary table
    def generate_table(data):
        tables = {}
        for test_type in data["TestType"].unique():
            # Filter data for the current test type
            test_data = data[data["TestType"] == test_type]

            # Prepare an empty summary DataFrame
            summary_data = {"Metric": [
                "Min (Set)", "**Avg** (Set)", "Max (Set)",
                "Min (Get)", "**Avg** (Get)", "Max (Get)"
            ]}

            # Compute min, avg, max for each execution
            for execution in test_data["Execution"].unique():
                execution_data = test_data[test_data["Execution"] == execution]
                min_set = execution_data["SetTimeSeconds"].min()
                avg_set = execution_data["SetTimeSeconds"].mean()
                max_set = execution_data["SetTimeSeconds"].max()
                min_get = execution_data["GetTimeSeconds"].min()
                avg_get = execution_data["GetTimeSeconds"].mean()
                max_get = execution_data["GetTimeSeconds"].max()

                # Add to the summary
                summary_data[execution] = [
                    table_formater(min_set), f"**{table_formater(avg_set)}**", table_formater(max_set),
                    table_formater(min_get), f"**{table_formater(avg_get)}**", table_formater(max_get)
                ]

            # Convert summary data to DataFrame
            summary_df = pd.DataFrame(summary_data)

            # Convert to Markdown
            markdown_table = summary_df.to_markdown(index=False, tablefmt="github")

            # Store the Markdown table with label
            tables[f"{test_type}"] = markdown_table

        return tables

    # Generate tables for single-threaded and parallel executables
    markdown_tables.append(generate_table(single_threaded))
    markdown_tables.append(generate_table(parallel))

    return markdown_tables

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Visualizes the provided benchmark data."
    )

    parser.add_argument(
        "-i", "--input",
        type=str,
        required=True,
        help="Path to the output file."
    )

    args = parser.parse_args()

    input_path = args.input
    print(f"Output file specified: {input_path}")
    data = load_data(input_path)
    #plot_boxplots_with_custom_labels(data)
    tables = generate_tables(data)
    print("START PRINTING MARKDOWN:\n\n")
    for x in tables[0]:
        print(f"### Benchmark: {x}\n")
        print(tables[0][x])
        print("\n")
        print(tables[1][x])
        print("\n")
    print("\n\nEND PRINTING MARKDOWN:")
