defmodule Mix.Tasks.Compile.ScenicDriverDebug do
  use Mix.Task

  # import IEx

  @moduledoc """
  Automatically sets the SCENIC_LOCAL_TARGET for the Makefile
  """

  @mix_target (case function_exported?(Mix.Nerves.Utils, :mix_target, 0) do
                 true -> Mix.Nerves.Utils.mix_target()
                 false -> Mix.target()
               end)

  @spec target() :: atom
  def target(), do: @mix_target

  @spec run(OptionParser.argv()) :: {:ok, []} | no_return
  def run(_args) do
    {:ok, []}
  end
end
