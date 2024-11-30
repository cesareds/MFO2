#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "bank.hpp"

using json = nlohmann::json;
using namespace std;

using BankMap = map<string, BankState>;

enum class Action
{
  Init,
  Deposit,
  Withdraw,
  Transfer,
  BuyInvestment,
  SellInvestment,
  Unknown
};

Action stringToAction(const std::string &actionStr)
{
  static const std::unordered_map<std::string, Action> actionMap = {
      {"init", Action::Init},
      {"deposit_action", Action::Deposit},
      {"withdraw_action", Action::Withdraw},
      {"transfer_action", Action::Transfer},
      {"buy_investment_action", Action::BuyInvestment},
      {"sell_investment_action", Action::SellInvestment}};

  auto it = actionMap.find(actionStr);
  if (it != actionMap.end())
  {
    return it->second;
  }
  else
  {
    return Action::Unknown;
  }
}

int int_from_json(json j)
{
  string s = j["#bigint"];
  return stoi(s);
}

map<string, int> balances_from_json(json j)
{
  map<string, int> m;
  for (auto it : j["#map"])
  {
    m[it[0]] = int_from_json(it[1]);
  }
  return m;
}

map<int, Investment> investments_from_json(json j)
{
  map<int, Investment> m;
  for (auto it : j["#map"])
  {
    m[int_from_json(it[0])] = {.owner = it[1]["owner"],
                               .amount = int_from_json(it[1]["amount"])};
  }
  return m;
}

BankState bank_state_from_json(json state)
{
  map<string, int> balances = balances_from_json(state["balances"]);
  map<int, Investment> investments =
      investments_from_json(state["investments"]);
  int next_id = int_from_json(state["next_id"]);
  return {.balances = balances, .investments = investments, .next_id = next_id};
}

//////////

void print_balancos(map<string, int> &b) // Criamos
{
  cout << "Balances: " << endl;
  for (auto it : b)
  {
    cout << "Nome: " << it.first << " | Valor: " << it.second << endl;
  }
}
void print_investimentos(map<int, Investment> &i) // Criamos
{
  cout << "Investimentos: " << endl;
  for (auto it : i)
  {
    cout << "Id: " << it.first
         << " | Dono: " << it.second.owner
         << " | Valor: " << it.second.amount
         << endl;
  }
}

void print_bank_state(BankState &bank_state) // Criamos
{
  cout
      << "Next Id: " << bank_state.next_id
      << endl;
  print_balancos(bank_state.balances);
  print_investimentos(bank_state.investments);
}

int main()
{
  for (int i = 0; i < 10000; i++)
  {
    cout << "Trace #" << i << endl;
    std::ifstream f("traces/out" + to_string(i) + ".itf.json");
    json data = json::parse(f);

    // Estado inicial: começamos do mesmo estado incial do trace
    BankState bank_state =
        bank_state_from_json(data["states"][0]["bank_state"]);

    auto states = data["states"];
    for (auto state : states)
    {
      string action = state["action_taken"];
      json nondet_picks = state["nondet_picks"];

      string error = "";

      // Próxima transição
      switch (stringToAction(action))
      {
      case Action::Init:
      {
        cout << "initializing" << endl;
        break;
      }

      case Action::Deposit: // Lógica baseada no quint
      {
        string depositor = nondet_picks["depositor"]["value"];
        int amount = int_from_json(nondet_picks["amount"]["value"]);
        print_bank_state(bank_state);
        cout << "deposit(" << bank_state.next_id << ", "<< depositor << "," << amount << ")" << endl;
        error =  deposit(bank_state, depositor, amount);
        break;
      }
      
      case Action::Withdraw: // Lógica baseada no quint
      {
        string withdrawer = nondet_picks["withdrawer"]["value"];
        int amount = int_from_json(nondet_picks["amount"]["value"]);
        cout << "withdraw(" << bank_state.next_id << ", " << withdrawer
             << ", " << amount << ")" << endl;
        print_bank_state(bank_state);
        error = withdraw(bank_state, withdrawer, amount);
        break;
      }



      case Action::Transfer: // Lógica baseada no quint
      {
        string sender = nondet_picks["sender"]["value"];
        string receiver = nondet_picks["receiver"]["value"];
        int amount = int_from_json(nondet_picks["amount"]["value"]);
        print_bank_state(bank_state);
        cout << "transfer(" << bank_state.next_id << ", " << sender
             << receiver << amount << ")" << endl;

        error = transfer(bank_state, sender, receiver, amount);
        break;
      }

      case Action::BuyInvestment: // Lógica baseada no quint
      {
        string buyer = nondet_picks["buyer"]["value"];
        int amount = int_from_json(nondet_picks["amount"]["value"]);
        print_bank_state(bank_state);
        cout << "buy_investment(" << bank_state.next_id << ", " << buyer
             << ", " << amount << ")" << endl;

        error = buy_investment(bank_state, buyer, amount);
        break;
      }


      case Action::SellInvestment:
      {
        string seller = nondet_picks["seller"]["value"];
        cout << "seller: " << seller << endl;
        int id = int_from_json(nondet_picks["id"]["value"]);
        cout << "investment_id: " << id << endl;

        print_bank_state(bank_state);
        cout << "sell_investment(" << bank_state.next_id << "," << seller << ","  << id << ")" << endl;
        error = sell_investment(bank_state, seller, id);
        break;
      }






      default:
      {
        cout << "Unknown action " << action << endl;
        error = "";
        break;
      }
      }

      BankState expected_bank_state = bank_state_from_json(state["bank_state"]);

      string expected_error = string(state["error"]["tag"]).compare("Some") == 0
                                  ? state["error"]["value"]
                                  : "";

      cout << "-------------------- Expected --------------------------------"
           << endl;
      print_bank_state(expected_bank_state);
      cout << "-------------------- Actual ----------------------------------"
           << endl;
      print_bank_state(bank_state);
      cout << "--------------------------------------------------------------"
           << endl;
      cout << endl;

      cout << "Erro esperado: " << expected_error << endl;
      cout << "Erro obtido:   " << error << endl;
      cout << "----------------------------------------------------"
           << endl;

      assert(expected_error == error); // Asserts
      assert(expected_bank_state.balances == bank_state.balances); // Assert
    }
  }
  return 0;
}







